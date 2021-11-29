## Compilation using vcpkg

First of all, clone [vcpkg](https://github.com/microsoft/vcpkg) to a local directory on your system.

```
git clone https://github.com/microsoft/vcpkg
```


### Setup on Linux

As the first step, call the following command in the directory which vcpkg has been cloned to.

```
export VCPKG_HOME=$(pwd)
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

NOTE: It seems like currently the vcpkg version of GLEW requires some packages to be already installed using the system
package manager in order to compile it. On Ubuntu for example, the following command can help install all dependencies.

```
sudo apt install libxmu-dev libxi-dev libgl-dev libglu1-mesa-dev
```


### Setup on Windows

As the first step, please call the following command in the directory which vcpkg has been cloned to (assuming the
PowerShell is used and not cmd.exe).

```
$env:VCPKG_HOME = "${PWD}"
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

On Windows, installing the Boost.Interprocess package is necessary.
Please do not forget to add `--triplet=x64-windows` if the 64-bit version of the package should be installed.

```
./vcpkg install boost-interprocess
```

Then, the program can be built using the following commands. Please adapt the paths where necessary.

```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_HOME/scripts/buildsystems/vcpkg.cmake" -DCMAKE_INSTALL_PREFIX=<path> ..
cmake --build . --parallel
cmake --build . --target install
```

Hint: To change the language of warnings and error messages to English even if your system uses another language,
consider setting the environment variable `set VSLANG=1033`.
