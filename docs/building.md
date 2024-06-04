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
1. Install pacman
    * `wget https://github.com/devkitPro/pacman/releases/download/v1.0.2/devkitpro-pacman.amd64.deb`
    * `sudo apt-get install gdebi-core`
    * `sudo gdebi devkitpro-pacman.amd64.deb`
1. Install devkitpro
    * `sudo dkp-pacman -Syu`
    * `sudo dkp-pacman -S switch-dev switch-glm switch-libjpeg-turbo devkitARM devkitarm-rules hactool`
    * `echo export DEVKITPRO="/opt/devkitpro" >> .bashrc`
1. Install Python 2
    * `sudo apt install python2 zip`
    * `curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py`
    * `sudo python2 get-pip.py`
    * `pip install lz4 pycryptodome`
    * `sudo update-alternatives --install /usr/bin/python python /usr/bin/python2.7 1`
1. Finally, clone the Atmosphère repository and run `make` under its root directory.