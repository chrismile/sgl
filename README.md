## sgl - Simple Graphics Library (using OpenGL and SDL)

sgl is a collection of utility functions for developing OpenGL graphics applications for Linux & Windows with C++.

## Usage

The most recommended way of learning to use this library is reading and understanding the sample code in the directory 'samples' (still work in progress).

If you built the doxygen documentation for this project (for more details see the next section), you can also look there for more information on classes and interfaces.


## Compilation

On Ubuntu 18.04 for example, you can install all necessary packages with this command:

```
sudo apt-get install git cmake libglm-dev libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libglew-dev libpng-dev libboost-filesystem-dev libtinyxml2-dev
```

Similar packages should also be available on other distributions.

To start the compilation, launch the following commands in the directory of the project:

```
mkdir build
cd build
rm -rf *
cmake ..
make
```

To install the library to /usr/local, run 

```
sudo make install
```

If doxygen is installed on your system, you can call

```
doxygen doxygen.conf
```

in the main directory to generate the documentation for this project.


## Compilation on Windows

The build process was tested on Windows 10 64-bit using MSYS2 and Mingw-w64 (http://www.msys2.org/). Using MSYS2 and Pacman, the following packages need to be installed.

```
pacman -S make git wget mingw64/mingw-w64-x86_64-gcc wget mingw64/mingw-w64-x86_64-gdb
pacman -S mingw64/mingw-w64-x86_64-glm mingw64/mingw-w64-x86_64-libpng mingw64/mingw-w64-x86_64-SDL2 mingw64/mingw-w64-x86_64-SDL2_image mingw64/mingw-w64-x86_64-SDL2_mixer mingw64/mingw-w64-x86_64-SDL2_ttf mingw64/mingw-w64-x86_64-tinyxml2 mingw64/mingw-w64-x86_64-boost mingw64/mingw-w64-x86_64-glew mingw64/mingw-w64-x86_64-cmake
```

Finally, when SGL has been cloned, it needs to be compiled and installed to, e.g., /usr/local.

```
mkdir build
cd build
rm -rf *
cmake .. -G"MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
make install
export PATH=$PATH:"/c/msys64/usr/local/lib"
```

Please note that, when launching programs using SGL on Windows, either the library path of SGL (i.e., /usr/local) needs to be included in the PATH variable, or the DLL file needs to be copied to the application directory containing the executable. When launching the program outside of the MSYS shell, the MinGW/MSYS DLL directories also need to be included in the PATH variable.


## License

Copyright (c) 2017, Christoph Neuhauser

BSD 3-Clause License (for more details see LICENSE file)
