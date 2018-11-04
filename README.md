## open bugs
### 1  on osx: using 
        `$ sh sandbash/selftest_gapped2.sh runcmake_debug.sh` 
        the filenames of the received files are truncated
### 2 on all platforms
        some paramter combinations result in crash of the modem
        TODO list them
    
```
  ____          _       _                
 / ___|_ __ ___| |_ ___| |__   ___ _ __  
| |  _| '__/ _ \ __/ __| '_ \ / _ \ '_ \ 
| |_| | | |  __/ || (__| | | |  __/ | | |
 \____|_|  \___|\__\___|_| |_|\___|_| |_|
```
## Dependencies
cmake           https://cmake.org  
ODER:  
```sudo apt-get install cmake```  
autoconf:  
```sudo apt-get install autoconf```
#### Audio backend
portaudio       http://www.portaudio.com/
#### Static dependencies
libfec          https://github.com/quiet/libfec  
liquid sdr      https://github.com/jgaeddert/liquid-dsp/  

## Building
After building and copying the static libraries to `libs/external/LIBNAME` (see below), 
the project is simply build by running the `runcmake.sh` file (on posix and OSX).

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
#### portaudio
The portaudio package is available in most distributions.  
If its not available, follow their instructions of how to install it systemwide. 

### osx
#### libfec
```
$ ./configure --build=x86_64-apple-darwin
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
#### portaudio
Portaudio can be simply be installed system wide via homebrew `https://brew.sh`.  
```
$ brew install portaudio
```

### windows
