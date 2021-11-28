#ifndef __TRACYOPENGL_HPP__
#define __TRACYOPENGL_HPP__

#if !defined GL_TIMESTAMP && !defined GL_TIMESTAMP_EXT
#  error "You must include OpenGL 3.2 headers before including TracyOpenGL.hpp"
#endif

#if !defined TRACY_ENABLE || defined __APPLE__

#define TracyGpuContext
#define TracyGpuContextName(x,y)
#define TracyGpuNamedZone(x,y,z)
#define TracyGpuNamedZoneC(x,y,z,w)
#define TracyGpuZone(x)
#define TracyGpuZoneC(x,y)
#define TracyGpuZoneTransient(x,y,z)
#define TracyGpuCollect

#define TracyGpuNamedZoneS(x,y,z,w)
#define TracyGpuNamedZoneCS(x,y,z,w,a)
#define TracyGpuZoneS(x,y)
#define TracyGpuZoneCS(x,y,z)
#define TracyGpuZoneTransientS(x,y,z,w)

namespace tracy
{
struct SourceLocationData;
class GpuCtxScope
{
public:
    GpuCtxScope( const SourceLocationData*, bool ) {}
    GpuCtxScope( const SourceLocationData*, int, bool ) {}
};
}

#else

#error Tracy support was not enabled or the Tracy submodule was not initialized.

#endif

#endif
