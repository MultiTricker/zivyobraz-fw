#!/usr/bin/env python3
"""
Generate .clangd configuration from PlatformIO's c_cpp_properties.json
Run this script after PlatformIO updates the c_cpp_properties.json file.
"""

import json
import os
import re
import sys
from pathlib import Path

# Try to detect PlatformIO
try:
    Import("env")
    IS_PLATFORMIO = True
except:
    IS_PLATFORMIO = False


def remove_json_comments(text):
    """Remove C-style comments from JSON text"""
    # Remove single-line comments (// ...)
    text = re.sub(r'//.*', '', text)
    # Remove multi-line comments (/* ... */)
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    return text


def needs_regeneration(cpp_properties_path, clangd_config_path):
    """Check if .clangd needs to be regenerated based on file timestamps"""
    if not clangd_config_path.exists():
        return True

    cpp_mtime = cpp_properties_path.stat().st_mtime
    clangd_mtime = clangd_config_path.stat().st_mtime

    # Regenerate if c_cpp_properties.json is newer than .clangd
    return cpp_mtime > clangd_mtime


def generate_clangd_config():
    """Generate .clangd configuration from c_cpp_properties.json"""

    # Paths - workspace root is parent of scripts directory
    workspace_root = Path().cwd() if IS_PLATFORMIO else Path(__file__).parent.parent

    cpp_properties_path = workspace_root / ".vscode" / "c_cpp_properties.json"
    clangd_config_path = workspace_root / ".clangd"

    if not cpp_properties_path.exists():
        msg = f"{cpp_properties_path} not found"
        print(f"Warning: {msg} (skipping .clangd generation)")
        return

    # Check if regeneration is needed
    if not needs_regeneration(cpp_properties_path, clangd_config_path):
        return

    # Load c_cpp_properties.json (with comment support)
    with open(cpp_properties_path, 'r') as f:
        content = f.read()
        content = remove_json_comments(content)
        cpp_properties = json.loads(content)

    # Get the first configuration (usually "PlatformIO")
    if not cpp_properties.get('configurations'):
        print("Error: No configurations found in c_cpp_properties.json")
        sys.exit(1)

    config = cpp_properties['configurations'][0]

    # Extract data
    include_paths = [path for path in config.get('includePath', []) if path]
    defines = [define for define in config.get('defines', []) if define]
    cpp_standard = config.get('cppStandard', 'gnu++11')
    c_standard = config.get('cStandard', 'gnu99')
    compiler_path = config.get('compilerPath', '')

    # Detect toolchain C++ include paths from compiler path
    toolchain_cxx_includes = []
    if 'toolchain-xtensa' in compiler_path:
        # Extract toolchain base path
        import os
        toolchain_base = compiler_path.split('/bin/')[0] if '/bin/' in compiler_path else ''
        if toolchain_base:
            # Add ESP32 toolchain C++ includes
            cxx_base = f"{toolchain_base}/xtensa-esp32-elf/include/c++/8.4.0"
            toolchain_cxx_includes = [
                f"-isystem{cxx_base}",
                f"-isystem{cxx_base}/xtensa-esp32-elf",
            ]

    # Build .clangd configuration
    clangd_config = [
        "CompileFlags:",
        "  Add:",
        "    # Compiler flags",
        "    - \"-fno-rtti\"",
        f"    - \"-std={cpp_standard}\"",
        "",
    ]

    # Add toolchain C++ includes if found
    if toolchain_cxx_includes:
        clangd_config.append("    # ESP32 Toolchain C++ includes")
        for include in toolchain_cxx_includes:
            clangd_config.append(f"    - \"{include}\"")
        clangd_config.append("")

    clangd_config.extend([
        "    # Defines",
    ])

    # Add defines
    for define in defines:
        # Escape internal quotes by using single quotes for YAML string
        # or by properly escaping the define value
        if '"' in define:
            # For defines with embedded quotes, escape them properly
            # e.g., MBEDTLS_CONFIG_FILE="mbedtls/esp_config.h"
            # becomes: - '-DMBEDTLS_CONFIG_FILE="mbedtls/esp_config.h"'
            clangd_config.append(f"    - '-D{define}'")
        else:
            # Simple defines without quotes
            clangd_config.append(f'    - "-D{define}"')

    clangd_config.extend([
        "",
        "    # Include paths (using -isystem to suppress errors from these headers)",
    ])

    # Add include paths using -isystem instead of -I to suppress diagnostics
    for include_path in include_paths:
        clangd_config.append(f"    - \"-isystem{include_path}\"")

    # Add removal and diagnostics settings
    clangd_config.extend([
        "",
        "  Remove:",
        "    # Remove problematic flags that clangd doesn't understand",
        "    - \"-fstrict-volatile-bitfields\"",
        "    - \"-fno-tree-switch-conversion\"",
        "    - \"-mlongcalls\"",
        "    - \"-mtext-section-literals\"",
        "",
        "Diagnostics:",
        "  UnusedIncludes: None",
        "  MissingIncludes: None",
        "  ",
        "Index:",
        "  Background: Build",
        ""
    ])

    # Write .clangd file
    with open(clangd_config_path, 'w') as f:
        f.write('\n'.join(clangd_config))

if __name__ == '__main__' or IS_PLATFORMIO:
    try:
        generate_clangd_config()
    except Exception as e:
        print(f"Error: Failed to generate .clangd: {e}")
        if not IS_PLATFORMIO:
            sys.exit(1)
