  ____          _       _                
 / ___|_ __ ___| |_ ___| |__   ___ _ __  
| |  _| '__/ _ \ __/ __| '_ \ / _ \ '_ \ 
| |_| | | |  __/ || (__| | | |  __/ | | |
 \____|_|  \___|\__\___|_| |_|\___|_| |_|

### Dependencies
cmake           https://cmake.org  
#### Audio backend
portaudio       http://www.portaudio.com/
#### Static dependencies
libfec          https://github.com/quiet/libfec  
liquid sdr      https://github.com/jgaeddert/liquid-dsp/  

## Building
After building and copying the static libraries to `libs/external/LIBNAME` (see below),
the project is simply build by running the runcmake.sh file (on posix and OSX).

### posix
#### libfec
```
$ ./configure  
$ make
$ make install (as sudo)
```
Now you can copy the `libfec.a` file and `fec.h` to `libs/external/libfec`.
#### liquid sdr
```
$ ./bootstrap (if repo is cloned)
$ ./configure
```
Next edit the generated `config.h` file and set 
`#define HAVE_FFTW3_H 0`  
and 
`#define HAVE_LIBFFTW3F 0`  
to zero. To compile without fftw3 support. We won't need it.  
```
$ make 
```
Next copy the `libliquid.a` and the headerfiles `liquid.h` and `liquid.internal.h` into `libs/external/liquid/`.  
To build gretchen simply run:
```
$ sh runmake.sh
or
$ sh runmake_debug.sh
``` 

### osx
#### libfec
#### liquid sdr
### windows





