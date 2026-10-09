# M5Unit - COLOR

## Overview

Library for COLOR using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U009

COLOR is a color recognition unit integrating a TCS3472x RGBC 4-channel sensor. It communicates over I2C at address `0x29`, can detect color values and return RGB data, includes a white LED fill-light for more stable measurement, and provides a LEGO-compatible mounting hole for easy mechanical integration.

## Related Link
See also examples using conventional methods here.

- [Unit Color & Datasheet](https://docs.m5stack.com/en/unit/COLOR)

### Required Libraries:
- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-COLOR - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

- [PlotToSerial](examples/UnitUnified/UnitColor/PlotToSerial)  
Displays detected colors to serial and  screen.  
Includes examples of multiple color correction methods from raw values.  
Hold BtnA to switch the correction of the 4th color bar (gamma / pure hue / brightness normalization / color name / grayscale).
- [ViaPaHub](examples/UnitUnified/UnitColor/ViaPaHub)  
Detects colors through PaHub (ch:0) and displays them to serial and screen.  
Requires [M5Unit-HUB](https://github.com/m5stack/M5Unit-HUB).

### For ESP-IDF settings

> **NOTE:** The ESP-IDF native build (`idf.py`) targets ESP-IDF **5.1 or later** (5.x and 6.x).

Each example covers a single unit, so the examples need no Kconfig choice. Build them directly:

```sh
cd examples/UnitUnified/UnitColor/PlotToSerial
idf.py set-target esp32s3               # or esp32 / esp32c5 / esp32c6 / esp32h2 / esp32p4 / ...
idf.py build flash monitor
```

## Support via [PaHub](https://docs.m5stack.com/en/unit/Unit-PaHub%20v2.1)

|Unit|Support|Note|
|---|---|---|
|UnitColor|OK||

See also the [ViaPaHub](examples/UnitUnified/UnitColor/ViaPaHub) example and [M5Unit-HUB](https://github.com/m5stack/M5Unit-HUB)

## Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-COLOR/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxygen](https://www.doxygen.nl/)
- [Git](https://git-scm.com/) (Output commit hash to html)

