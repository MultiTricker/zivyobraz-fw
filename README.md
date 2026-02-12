# Živý obraz - firmware
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/MultiTricker/zivyobraz-fw)

Welcome to the Živý obraz repository with firmware for e-Paper development boards based on ESP32/ESP32-S3. Live Image is used to feed ePaper/e-Ink displays with image data from a web server, whether it is a PNG or a custom basic RLE format called Z1/Z2/Z3.

  * Basic information can be found on the project website: https://zivyobraz.eu/ (Czech)
  * Specific information regarding getting things work can be found in the documentation at: https://wiki.zivyobraz.eu/ (Czech)
  * Firmware supports many boards, you can find specific hardware for Živý obraz to buy at [LáskaKit](https://www.laskakit.cz/vyhledavani/?string=%C5%BEiv%C3%BD+obraz) and [Pájeníčko](https://pajenicko.cz/vyhledavani?search=%C5%BEiv%C3%BD%20obraz).

**The documentation also includes precompiled firmware for flashing several of the most common types of ePapers, or you can use the web installer in Edge/Chrome browsers at: https://zivyobraz.eu/?page=instalace**.
**The default password for the Wi-Fi that the board transmits after uploading the firmware is: `zivyobraz`.**

----

In brief, about custom compilation and settings in the firmware code:

You will need to have the following libraries installed:
```ini
lib_deps =
    zinggjm/GxEPD2@^1.6.6
    jnthas/Improv WiFi Library@^0.0.4
    bblanchon/ArduinoJson@^7.2.1
    adafruit/Adafruit GFX Library@^1.11.9
    adafruit/Adafruit SHT4x Library@^1.0.5
    adafruit/Adafruit BME280 Library@^2.3.0
    sparkfun/SparkFun SCD4x Arduino Library@^1.1.2
    sensirion/Sensirion I2C STCC4@^1.0.0
    kikuchan98/pngle@^1.1.0
```

In **platformio.ini**, comment out default build flags under section **common** (they are specified here for automatic compilation checks on GitHub), so you can use your own display type for compilation in next steps:
```ini
build_flags =
    -D COLOR_TYPE=BW      # Comment out this for your own display type enabled by you in display.h
    -D DISPLAY_TYPE=GDEW0154T8 # Also comment out this
```

In code **board.h** do not forget to uncomment:
1. Type of board used (ESPink_V2, ES3ink, ...)
2. Select connection type (http/https) which will be used for contacting the server by using `-DUSE_CLIENT_HTTP`
or `-DUSE_CLIENT_HTTPS` compiler flag. Default value is `https` variant.
3. **Streaming feature** is ENABLED by default for all ESP32 devices. This allows efficient buffering of image data in RAM before streaming to the display. To disable on resource-constrained devices, add `-D STREAMING_DISABLED` to your build_flags in **platformio.ini**
4. If you plan to connect one of the supported sensors via uŠup for reading temperature, humidity, and pressure/CO2 and sending the values to the server, uncomment in **sensor.h**
```c
// #define SENSOR
```
5. Display type has to be changed in **display.h** In the case of GRAYSCALE, you must remove `zinggjm/GxEPD2` from **platformio.ini** (just comment it out), otherwise there will be a library collision and the code will not work. In that case, `lib/GxEPD2_4G` will be used. For other displays (BW, 3C, 7C), leave `zinggjm/GxEPD2` active, you don't need to do anything with the 4G version.
```c
#define COLOR_TYPE=BW           // black and white
// #define COLOR_TYPE=3C        // 3 colors - black, white and red/yellow
// #define COLOR_TYPE=GRAYSCALE // grayscale - 4 colors
// #define COLOR_TYPE=7C        // 7 colors
```
6. Uncomment the definition of the specific ePaper you are putting into operation. This section begins at line `18`, and you need to select a specific display, e.g.:
```c
// BW
// #define DISPLAY_TYPE=GDEY0213B7 // 122x250, 2.13"
// #define DISPLAY_TYPE=GDEW042T2  // 400x300, 4.2"
#define DISPLAY_TYPE=GDEW075T7     // 800x480, 7.5"
```

After successfully compiling and flashing the board, continue with the documentation "Bringing your own ePaper to life":
https://wiki.zivyobraz.eu/doku.php?id=start#oziveni_vlastniho_epaperu

----

**Simple diagram of how it works**

![](how_it_works_diagram.webp)

----

## License info 

![CC BY-NC-SA 4.0](https://i.creativecommons.org/l/by-nc-sa/4.0/88x31.png)

ZivyObraz FW itself is licensed under a [Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License](http://creativecommons.org/licenses/by-nc-sa/4.0/):

Attribution—Noncommercial—Share Alike  
✖ | Sharing without ATTRIBUTION  
✖ | Commercial Use  
✖ | Free Cultural Works  
✖ | Meets Open Definition
