## Compilation using MSYS2

The build process was tested on Windows 10 64-bit using MSYS2 and Mingw-w64 (http://www.msys2.org/).
Using MSYS2 and Pacman, the following packages need to be installed.

```
pacman -S make git wget mingw64/mingw-w64-x86_64-gcc wget mingw64/mingw-w64-x86_64-gdb
pacman -S mingw64/mingw-w64-x86_64-glm mingw64/mingw-w64-x86_64-libpng mingw64/mingw-w64-x86_64-SDL2 \
mingw64/mingw-w64-x86_64-SDL2_image mingw64/mingw-w64-x86_64-SDL2_mixer mingw64/mingw-w64-x86_64-SDL2_ttf \
mingw64/mingw-w64-x86_64-tinyxml2 mingw64/mingw-w64-x86_64-boost mingw64/mingw-w64-x86_64-glew \
mingw64/mingw-w64-x86_64-cmake mingw64/mingw-w64-x86_64-libarchive
```

For Vulkan support, you also need the following libraries additionally to the Vulkan SDK (https://www.lunarg.com/vulkan-sdk/).

```
pacman -S mingw64/mingw-w64-x86_64-vulkan-headers mingw64/mingw-w64-x86_64-vulkan-loader \
mingw64/mingw-w64-x86_64-vulkan-validation-layers mingw64/mingw-w64-x86_64-shaderc
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
(e.g., C:/msys2/usr/local/bin) needs to be included in the PATH variable, or the DLL file needs to be copied to the
application directory containing the executable. When launching the program outside of the MSYS2 shell, the MinGW/MSYS2
DLL directories also need to be included in the PATH variable. To permanently modify the MSYS2 PATH variable,
/etc/profile needs to be edited.
