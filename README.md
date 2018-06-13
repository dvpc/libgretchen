  ____          _       _                
 / ___|_ __ ___| |_ ___| |__   ___ _ __  
| |  _| '__/ _ \ __/ __| '_ \ / _ \ '_ \ 
| |_| | | |  __/ || (__| | | |  __/ | | |
 \____|_|  \___|\__\___|_| |_|\___|_| |_|

The project consists of gretchen and gretchenbackend. The backend is 
used in the shell app (gret.c) to demonstrate the usage of gretechen.

### Dependencies
cmake           https://cmake.org  
#### For the audio backend  
portaudio       http://www.portaudio.com/
#### Static dependencies
libfec          https://github.com/quiet/libfec  
liquid sdr      https://github.com/jgaeddert/liquid-dsp/  

## Building
After building and copying the static libraries to libs/external/LIBNAME (see below),
the project is simply build by running the runcmake.sh file (on posix and OSX).

### posix
#### libfec
#### liquid sdr
### osx
#### libfec
#### liquid sdr
### windows





