## sgl - Simple Graphics Library (using OpenGL and SDL)

sgl is a collection of utility functions for developing OpenGL and Vulkan graphics applications for Linux & Windows with C++.


## Usage

The most recommended way of learning to use this library is reading and understanding the sample code in the directory
'samples' (still work in progress).

If you built the doxygen documentation for this project (for more details see the next section), you can also look there
for more information on classes and interfaces.


## Compilation

Currently, sgl supports three main ways to compile the library:
- Linux: Using the system package manager to install all dependencies (tested: apt on Ubuntu, pacman on Arch Linux).
- Linux & Windows: Installing all dependencies using [vcpkg](https://github.com/microsoft/vcpkg).
- Windows: Using MSYS2 to install all dependencies.

On Windows, we recommend to use vcpkg if you need to use Microsoft Visual Studio.
Guides for the different build types can be found in the directory `docs/compilation`.

If doxygen is installed on your system, you can call the following command in the main directory to generate the API
documentation for this project.

```
doxygen doxygen.conf
```


## License

Copyright (c) 2017-2021, Christoph Neuhauser

BSD 3-Clause License (for more details see LICENSE file)

Parts of the Vulkan code were created with the help of https://vulkan-tutorial.com/, who released their code under
public domain.
