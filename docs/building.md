# Building Atmosphère
Building Atmosphère is a very straightforward process that relies almost exclusively on tools provided by the [devkitPro](https://devkitpro.org) organization.

## Dependencies
+ [devkitA64](https://devkitpro.org)
+ [devkitARM](https://devkitpro.org)
+ [Python 2](https://www.python.org) (Python 3 may work as well, but this is not guaranteed)
+ [LZ4](https://pypi.org/project/lz4)
+ [PyCryptodome](https://pypi.org/project/pycryptodome) (optional)
+ [hactool](https://github.com/SciresM/hactool)

## Instructions
1. Follow the guide located [here](https://devkitpro.org/wiki/Getting_Started) to install and configure all the tools necessary for the build process.

2. Install the following packages via (dkp-)pacman:
    + `switch-dev`
    + `switch-glm`
    + `switch-libjpeg-turbo`
    + `devkitARM`
    + `devkitarm-rules`
    + `hactool`

3. Install the following library via python's package manager `pip`, required by [exosphere](components/exosphere.md):
    + `lz4`

4. Finally, clone the Atmosphère repository and run `make` under its root directory.
