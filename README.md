colrdx
======

This is a version of [colrdx](http://dev.man-online.org/man1/colrdx/) with enhancements to connect to [ON4KST chat](http://www.on4kst.com/chat/start.php) a.k.a. "KST". KST is a chat for Amateur Radio to arrange contacts in the higher bands of the spectrum where random contacts are rare or impossible due to the very narrow beamwidth of the antennas. The chat also allows the posting of noticeable contacts a.k.a. "DX spots" that help the others estimate the conditions of propagation.

It connects to the KST server via telnet on address `www.on4kst.info` and port `23000`.

Be aware that since the interface is telnet based your password will transit in clear thus you should choose a password dedicated to the ON4KST chat different from your other more secured passwords.

# Installation and usage
## Get the code
Just clone this repository

## Prerequisites
The configure script will yell if you are missing libncurses development library. In Debian distributions this is installed with:
  - `sudo apt-get install libncurses5-dev`
  - `sudo apt-get install cmake`

## Build
  - Make a build directory inside your clone
    - `mkdir build`
  - cd into it
    - `cd build`
  - run cmake
    - `cmake -DCMAKE_INSTALL_PREFIX=your_install_location ..`
  - run make with the number of threads corresponding to the logical CPUs of your machine. For example:
    - `make -j8`
  - run make install with sudo if you install the software in the default directory
    - `make install`

## Run
To connect to the KST chat you need to contact the `www.on4kst.info` server on port `23000`. Run it with your callsign in -c option and with -k option to get KST display enhancements:
  - `/opt/install/colrdx/bin/colrdx -c <my_call> -k www.on4kst.info 23000`

To check all options run the help
  - `/opt/install/colrdx/bin/colrdx -h`

Enter your password then you will see a screen prompting for chat room selection:

```
This telnet access is reserved to HAM only
Your IP address is xxx.xxx.xxx.xxx
Login:
Password:
xxxxxxxxxxxxxxxxxxxxxxxx

Chat selection ?
50/70 MHz..............1
144/432 MHz............2
Microwave..............3
EME/JT65...............4
Low Band...............5
50 MHz IARU Region 3...6
50 MHz IARU Region 2...7
144/432 MHz IARU R 2...8
144/432 MHz IARU R 3...9
kHz (2000-630m).......10
Warc (30,17,12m)......11
28 MHz................12
Your choice           :
2
Welcome /P 70cm F4EXB on this 144/432 MHz amateur chat (by ON4KST)

Use the inline ON4KST-2 CLX DX cluster for your spot.
More info type "/HELP"
```

Enter the number of your choice then you can enter your commands in the prompt area with the green background at the bottom of the screen

Enjoy!

