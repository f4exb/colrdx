colrdx
======

This is a version of colrdx with enhancements to connect to ON4KST chat a.k.a. "KST". KST is a chat for Amateur Radio to arrange contacts in the higher bands of the spectrum where random contacts are rare or impossible due to the very narrow beamwidth of the antennas. The chat also allows the posting of noticeable contacts a.k.a. "DX spots" that help the others to estimate the conditions of propagation.

# Installation and usage
## Get the code
Just clone this repository

## First time with the clone
You need to install the "auto tools": automake and autoconf. On Debian distributions just install automake and the rest will follow as a dependency:
  - `sudo apt-get install automake`

Then create the configure script and other things with the "auto tools". Just run the `bootstrap.sh` script.

## Build
  - Make a build directory inside your clone
    - `mkdir build`
  - cd into it
    - `cd build`
  - run the configure script optionally giving the install directory
    - `../configure --prefix=/opt/install/colrdx`
  - run make with the number of threads corresponding to the logical CPUs of your machine +1. Here on a core-i7:
    - `make -j9`
  - run make install with sudo if you install the software in the default directory
    - `make install`
    
## Run
To connect to the KST chat you need to contact the `www.on4kst.info` server on port `23000`. Run it with your callsign in -c option and with -k option to get KST display enhancements:
  - `/opt/install/colrdx/bin/colrdx -c <my_call> -k www.on4kst.info 23000`
  
Enjoy!

