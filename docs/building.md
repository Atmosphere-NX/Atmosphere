# Building Atmosphère
Building Atmosphère is a very straightforward process that relies almost exclusively on tools provided by the [devkitPro](https://devkitpro.org) organization.

## Dependencies
+ [devkitA64](https://devkitpro.org)
+ [devkitARM](https://devkitpro.org)
+ [Python 2 or 3](https://www.python.org)
+ [PyCryptodome](https://pypi.org/project/pycryptodome)

## Instructions
Follow the guide located [here](https://switchbrew.org/wiki/Setting_up_Development_Environment) to install and configure all the tools necessary for the build process. 

Install the following packages via (dkp-)pacman:
+ switch-dev
+ switch-libjpeg-turbo
+ devkitARM
+ devkitarm-rules

In order to build [Sept](components/sept.md) the pycryptodome PyPi package is required. You may install this package by running `pip install pycryptodome` under the installed Python environment of your choice. You may also want to install the zip package to support the `make dist` recipe.

Finally, clone the Atmosphère repository and run `make` under its root directory.
