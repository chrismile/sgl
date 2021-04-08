## sgl - Simple Graphics Library (using OpenGL and SDL)

sgl is a collection of utility functions for developing OpenGL and Vulkan graphics applications for Linux & Windows with C++.

## Usage

The most recommended way of learning to use this library is reading and understanding the sample code in the directory 'samples' (still work in progress).

If you built the doxygen documentation for this project (for more details see the next section), you can also look there for more information on classes and interfaces.


## Compilation

On Ubuntu 20.04 for example, you can install all necessary packages with this command:

```
sudo apt-get install git cmake libglm-dev libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libpng-dev libboost-filesystem-dev libtinyxml2-dev libarchive-dev
```

For OpenGL support, you also need the following libraries.

```
sudo apt-get install libglew-dev
```

For Vulkan support, you need to install the LunarG Vulkan SDK and shaderc. Below, an example for installing release 1.2.162 on Ubuntu 20.04 is given (see https://vulkan.lunarg.com/sdk/home#linux).

```
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.162-focal.list https://packages.lunarg.com/vulkan/1.2.162/lunarg-vulkan-1.2.162-focal.list
sudo apt update
sudo apt install vulkan-sdk
sudo apt install shaderc
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
pacman -S mingw64/mingw-w64-x86_64-glm mingw64/mingw-w64-x86_64-libpng mingw64/mingw-w64-x86_64-SDL2 mingw64/mingw-w64-x86_64-SDL2_image mingw64/mingw-w64-x86_64-SDL2_mixer mingw64/mingw-w64-x86_64-SDL2_ttf mingw64/mingw-w64-x86_64-tinyxml2 mingw64/mingw-w64-x86_64-boost mingw64/mingw-w64-x86_64-glew mingw64/mingw-w64-x86_64-cmake mingw64/mingw-w64-x86_64-libarchive
```

For Vulkan support, you also need the following libraries additionally to the Vulkan SDK (https://www.lunarg.com/vulkan-sdk/).

```
pacman -S mingw64/mingw-w64-x86_64-vulkan-headers mingw64/mingw-w64-x86_64-vulkan-loader mingw64/mingw-w64-x86_64-vulkan-validation-layers mingw64/mingw-w64-x86_64-shaderc
```

Finally, when sgl has been cloned, it needs to be compiled and installed to, e.g., /usr/local.

```
mkdir build
cd build
rm -rf *
cmake -G "MSYS Makefiles" -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
make install
export PATH=$PATH:"/c/msys64/usr/local/bin"
```

Please note that, when launching programs using sgl on Windows, either the library path of sgl 
(i.e., /usr/local/bin) needs to be included in the PATH variable, or the DLL file needs to be 
copied to the application directory containing the executable. When launching the program 
outside of the MSYS shell, the MinGW/MSYS DLL directories also need to be included in the PATH 
variable. To permanently modify the MSYS PATH variable, /etc/profile needs to be edited.


## License

Copyright (c) 2017-2021, Christoph Neuhauser

BSD 3-Clause License (for more details see LICENSE file)
