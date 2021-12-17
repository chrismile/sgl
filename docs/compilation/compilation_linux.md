## Compilation on Linux

### Ubuntu

The build process was tested on Ubuntu 18.04 and Ubuntu 20.04.

All obligatory dependencies can be installed using the following command.

```
sudo apt-get install git cmake libglm-dev libsdl2-dev libsdl2-image-dev libpng-dev libboost-filesystem-dev \
libtinyxml2-dev libarchive-dev
```

For OpenGL support (which is recommended), you also need the following libraries.

```
sudo apt-get install libglew-dev
```

For Vulkan support, the LunarG Vulkan SDK and shaderc need to be installed. Below, an example for installing release
1.2.162 on Ubuntu 20.04 is given (see https://vulkan.lunarg.com/sdk/home#linux).

```
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.162-focal.list \
https://packages.lunarg.com/vulkan/1.2.162/lunarg-vulkan-1.2.162-focal.list
sudo apt update
sudo apt install vulkan-sdk
sudo apt install shaderc
```

To start the compilation, launch the following commands in the directory of the project:

```
mkdir build
cd build
rm -rf *
cmake ..
make -j
```

To install the library to /usr/local, run

```
sudo make install
```

In case you wish to install the library to any other directory, specify `-DCMAKE_INSTALL_PREFIX=<path>` when calling
`cmake`.


### Arch Linux

The following command can be used to install all dependencies on Arch Linux (last tested in May 2021).

```
sudo pacman -S git cmake boost libarchive glm tinyxml2 sdl2 sdl2_image glew vulkan-devel shaderc
```

Vulkan support on Arch Linux was not yet thoroughly tested. Please open a bug report in the issue tracker in case you
face any problems.

All other build instructions are identical to the ones for Ubuntu provided above.
