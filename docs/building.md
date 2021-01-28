# Building Atmosphère
Building Atmosphère is a very straightforward process that relies almost exclusively on tools provided by the [devkitPro](https://devkitpro.org) organization.

## Dependencies
+ [devkitA64](https://devkitpro.org)
+ [devkitARM](https://devkitpro.org)
+ [Python 2](https://www.python.org) (Python 3 may work as well, but this is not guaranteed)
+ [LZ4](https://pypi.org/project/lz4)
+ [PyCryptodome](https://pypi.org/project/pycryptodome) (optional)

## Instructions
1. Follow the guide located [here](https://devkitpro.org/wiki/Getting_Started) to install and configure all the tools necessary for the build process. 

2. Install the following packages via (dkp-)pacman:
    + `switch-dev`
    + `switch-glm`
    + `switch-libjpeg-turbo`
    + `devkitARM`
    + `devkitarm-rules`

3. Install the following library via python's package manager `pip`, required by [exosphere](components/exosphere.md):
    + `lz4`

4. (Optional) In order to build [sept](components/sept.md) the pycryptodome PyPi package is required, which can be installed by running `pip install pycryptodome` under the installed Python environment of your choice or by installing the complete zip package to support the `make dist` recipe. This is an optional step included for advanced users who have the ability to provide the necessary encryption/signing keys themselves.

5. It is, instead, possible to build [sept](components/sept.md) by providing previously encrypted/signed binaries distributed by official Atmosphère release packages. In order to do so, export the following variables in your current environment:
    + `SEPT_00_ENC_PATH` (must point to the `sept-secondary_00.enc` file)
    + `SEPT_01_ENC_PATH` (must point to the `sept-secondary_01.enc` file)
    + `SEPT_DEV_00_ENC_PATH` (must point to the `sept-secondary_dev_00.enc` file)
    + `SEPT_DEV_01_ENC_PATH` (must point to the `sept-secondary_dev_01.enc` file)

6. Finally, clone the Atmosphère repository and run `make` under its root directory.
