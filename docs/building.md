# Building Atmosphère
The process for building Atmosphère is similar to building Fusée Gelée payloads and other Switch apps.

In order to build Atmosphère you must have devkitARM and devkitA64 installed on your computer. You can find instructions on how to install and setup devkitARM and devkitA64 on various OSes [here](https://devkitpro.org/wiki/Getting_Started). You'll need to install the following packages via (dkp-)pacman: switch-dev switch-freetype devkitARM devkitarm-rules

sept requires you have python installed with the pycryptodome PyPi packages (`pip install pycryptodome`). You may also want to install the zip package from your package manager of choice to support the `make dist` recipe.

Once you have finished installing the devkitPro-provided toolchain/libraries, python, and the dependencies, simply clone the Atmosphère repo (clone with the -r flag), change your directory to it and run `make`.
