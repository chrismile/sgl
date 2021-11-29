## Profiling with Tracy

Tracy (https://github.com/wolfpld/tracy) can be used for profiling of sgl and programs using sgl. `git submodule init`
and `git submodule update` must be called in order to download Tracy to the folder `submodules/tracy/`.
To enable Tracy support, please pass `-DTRACY_ENABLE=ON` to CMake when configuring the project build. This enables the
integration of the Tracy client into sgl.

In order to build the profiling server used to display profiling information, please refer to the documentation of
Tracey (https://github.com/wolfpld/tracy/releases/latest/download/tracy.pdf). On Ubuntu 20.04, for example, the
following dependencies need to be installed using the system package manager.

```
sudo apt-get install libcapstone-dev libglfw3-dev libfreetype-dev libgtk-3-dev
```

Then, `make` can be used in the directory `submodules/tracy/profiler/build/unix/` to build the profiler server.
