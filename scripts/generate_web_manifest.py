import os
import json
import shutil

Import("env")

def generate_manifest(source, target, env):
    # Klíč "custom_fw_name" - jinak název projektu/prostředí.
    fw_name = env.GetProjectOption("custom_fw_name", env.get("PROGNAME")).replace("'", "")

    # Načtení verze, 2.0 jako výchozí
    fw_version = env.GetProjectOption("custom_fw_version", "2.0").replace("'", "")

    # Zbytek nastavení
    board_config = env.BoardConfig()
    mcu = board_config.get("build.mcu", "esp32")
    prog_name = env.get("PROGNAME") + ".bin"
    build_dir = env.subst("$BUILD_DIR")

    # Logika pro offsety
    if "esp32s3" in mcu:
        bootloader_offset = 0x0
        chip_family = "ESP32-S3"
    elif "esp32c3" in mcu:
        bootloader_offset = 0x0
        chip_family = "ESP32-C3"
    else:
        bootloader_offset = 0x1000
        chip_family = "ESP32"

    # Cesty k souborům
    firmware_path = os.path.join(build_dir, prog_name)
    partitions_path = os.path.join(build_dir, "partitions.bin")
    bootloader_path = os.path.join(build_dir, "bootloader.bin")
    boot_app0_target = os.path.join(build_dir, "boot_app0.bin")

    # Získání boot_app0 (fallback)
    if not os.path.exists(boot_app0_target):
        try:
            platform = env.PioPlatform()
            package_dir = platform.get_package_dir("framework-arduinoespressif32")
            if package_dir:
                boot_app0_source = os.path.join(package_dir, "tools", "partitions", "boot_app0.bin")
                if os.path.exists(boot_app0_source):
                    shutil.copy(boot_app0_source, boot_app0_target)
        except Exception:
            pass

    # --- SESTAVENÍ JSON MANIFESTU ---
    parts_full = []

    if os.path.exists(bootloader_path):
        parts_full.append({"path": "bootloader.bin", "offset": bootloader_offset})

    if os.path.exists(partitions_path):
        parts_full.append({"path": "partitions.bin", "offset": 0x8000})

    if os.path.exists(boot_app0_target):
        parts_full.append({"path": "boot_app0.bin", "offset": 0xe000})

    parts_full.append({"path": prog_name, "offset": 0x10000})

    # POUŽITÍ NAČTENÉHO NÁZVU (fw_name)
    manifest_full = {
        "name": fw_name,
        "version": fw_version,
        "new_install_prompt_erase": True,
        "improv": True,
        "builds": [{"chipFamily": chip_family, "parts": parts_full}]
    }

    try:
        with open(os.path.join(build_dir, "manifest_full.json"), "w", encoding="utf-8") as f:
            json.dump(manifest_full, f, indent=2, ensure_ascii=False)

        print(f"Manifesty pro '{fw_name}' vygenerovány.")
    except Exception as e:
        print(f"Chyba JSON: {e}")

env.AddPostAction("buildprog", generate_manifest)
