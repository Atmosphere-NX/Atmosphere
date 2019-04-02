# Building Atmosphère
The process for building Atmosphère is similar to building Fusée Gelée payloads and other Switch apps.

In order to build Atmosphère you must have devkitARM and devkitA64 installed on your computer. You can find instructions on how to install and setup devkitARM and devkitA64 on various OSes [here](https://devkitpro.org/wiki/Getting_Started). If you're using pacman or dkp-pacman you'll need to install the following packages: devkitARM devkitarm-rules devkitA64 general-tools switch-tools libnx switch-freetype

sept requires you have python installed with the pycrypto and pycryptodome PyPi packages (pip install pycrypto pycryptodome). You may also want to install the zip package from your package manager of choice to support the `make dist` recipe.

Once you have finished installing devkitPro, python, and the dependencies, simply clone the Atmosphère repo (clone with the -r flag), change your directory to it and run `make`.
