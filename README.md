## sgl - Simple Graphics Library (using OpenGL and SDL)

sgl is a collection of utility functions for developing OpenGL graphics applications for Linux & Windows with C++.

## Usage

The most recommended way of learning to use this library is reading and understanding the sample code in the directory 'samples' (still work in progress).
If you built the doxygen documentation for this project (for more details see the next section), you can also look there for more information on classes and interfaces.


## Compilation

On Ubuntu 17.04 for example, you can install all necessary packages with this command:

`sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libglew-dev libpng-dev libboost-filesystem-dev libtinyxml2-dev`

Similar packages should also be available on other distributions.

To start the compilation, launch the following commands in the directory of the project:

`cd build
rm -rf *
cmake ..
make`

If doxygen is installed on your system, you can call
`doxygen doxygen.conf`
to generate the documentation for this project.


## License

Copyright (c) 2017, Christoph Neuhauser

BSD 3-Clause License (for more details see LICENSE file)