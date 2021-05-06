## Compilation using vcpkg

First of all, clone [vcpkg](https://github.com/microsoft/vcpkg) to a local directory on your system.

```
git clone https://github.com/microsoft/vcpkg
```


### Setup on Linux

As the first step, call the following command in the directory which vcpkg has been cloned to.

```
./bootstrap-vcpkg.sh -disableMetrics
```

For Vulkan support, the LunarG Vulkan SDK needs to be installed. Below, an example for installing release 1.2.162 on
Ubuntu 20.04 is given (see https://vulkan.lunarg.com/sdk/home#linux).

```
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-1.2.162-focal.list \
https://packages.lunarg.com/vulkan/1.2.162/lunarg-vulkan-1.2.162-focal.list
sudo apt update
sudo apt install vulkan-sdk
```

On Arch Linux, the following command can be used.

```
sudo pacman -S vulkan-devel
```

After that, the environment variable `VULKAN_SDK` needs to be set to the installation directory of the Vulkan SDK, e.g.:

```
export VULKAN_SDK=/usr
```


### Setup on Windows

As the first step, please call the following command in the directory which vcpkg has been cloned to.

```
export VCPKG_HOME=${PWD}
./bootstrap-vcpkg.bat -disableMetrics
```

If the user wants to compile sgl with Vulkan support, the Vulkan SDK needs to be installed from:
https://vulkan.lunarg.com/sdk/home#windows

The installation process of the Vulkan SDK should set the environment variable `VULKAN_SDK` for vcpkg.


### Installing all Packages (All Systems)

All necessary packages can be installed using the following command.
On Windows `--triplet=x64-windows` needs to be added if the 64-bit version of the packages should be installed.

```
./vcpkg install boost-core boost-algorithm boost-filesystem boost-locale libpng sdl2[vulkan] sdl2-image \
tinyxml2 glew glm libarchive[bzip2,core,lz4,lzma,zstd] vulkan vulkan-headers shaderc
```

If Vulkan support should be disabled, remove `vulkan`, `vulkan-headers` and `shaderc` from the command above.


### Compilation on Linux

To invoke the build process using CMake, the following commands can be used.
Please adapt `CMAKE_INSTALL_PREFIX` depending on which path sgl should be installed to.

```
mkdir build
cd build
rm -rf *
cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_HOME/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=<path> ..
make -j
make install
```


### Compilation on Windows

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_HOME/scripts/buildsystems/vcpkg.cmake -DCMAKE_INSTALL_PREFIX=<path> ..
cmake --build
cmake --install
```
