# M5Unit - COLOR

## Overview

Library for COLOR using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U009

COLOR is color recognition unit integrated with TCS3472 chipset. As the name says, COLOR is able to detect color value and return RGB data as response.

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


### Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-COLOR/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

#### Required
- [Doxyegn](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)


