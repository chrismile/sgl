#ifndef __TRACYOPENCL_HPP__
#define __TRACYOPENCL_HPP__

#if !defined TRACY_ENABLE

#define TracyCLContext(c, x) nullptr
#define TracyCLDestroy(c)
#define TracyCLContextName(c, x, y)

#define TracyCLNamedZone(c, x, y, z)
#define TracyCLNamedZoneC(c, x, y, z, w)
#define TracyCLZone(c, x)
#define TracyCLZoneC(c, x, y)

#define TracyCLNamedZoneS(c, x, y, z, w)
#define TracyCLNamedZoneCS(c, x, y, z, w, v)
#define TracyCLZoneS(c, x, y)
#define TracyCLZoneCS(c, x, y, z)

#define TracyCLNamedZoneSetEvent(x, e)
#define TracyCLZoneSetEvent(e)

#define TracyCLCollect(c)

namespace tracy
{
    class OpenCLCtxScope {};
}

using TracyCLCtx = void*;

#else

#error Tracy support was not enabled or the Tracy submodule was not initialized.

#endif

#endif
