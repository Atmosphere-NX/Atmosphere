# Building Atmosphère
The process for building Atmosphère is similar to building Fusée Gelée payloads and other Switch apps.

In order to build Atmosphère you must have devkitARM and devkitA64 installed on your computer. You can find instructions on how to install and setup devkitARM and devkitA64 on various OSes [here](https://devkitpro.org/wiki/Getting_Started). You'll need to install the following packages via (dkp-)pacman: switch-dev switch-freetype devkitARM devkitarm-rules . You will also need the latest libnx master. Get it thru git clone --recursive https://github.com/switchbrew/libnx.git && make install.

sept requires you have python installed with the pycrypto and pycryptodome PyPi packages (pip install pycrypto pycryptodome). You may also want to install the zip package from your package manager of choice to support the `make dist` recipe. Also make sure you harvest a sept-secondary.enc and put it in the sept folder in the cloned git.

Once you have finished installing the devkitPro-provided toolchain/libraries, python, and the dependencies, simply clone the Atmosphère repo (clone with the -r flag), change your directory to it and run `SEPT_ENC_PATH=/path/to/sept-secondary.enc make` else your sept simply won't compile.
