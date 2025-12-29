## HIP SDK headers

The files in this directory were obtained from https://github.com/ROCm/rocm-systems and are licensed under the terms
of the MIT license (see LICENSE.md).

### How to update?

- Call the two commands below.
```
git clone https://github.com/ROCm/rocm-systems.git
cd rocm-systems
```
- Call `git rev-parse --short HEAD` to retrieve the `HIP_VERSION_GITHASH` entry for `hip_version.h`.
- Update the version defines in `hip_version.h` with the content of `projects/hip/VERSION`.
- Copy the contents of `projects/hip/include/hip` and `projects/clr/hipamd/include/hip/amd_detail` to the respective
  directories.
