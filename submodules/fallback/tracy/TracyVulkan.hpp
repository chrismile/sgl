#ifndef __TRACYVULKAN_HPP__
#define __TRACYVULKAN_HPP__

#if !defined TRACY_ENABLE

#define TracyVkContext(x,y,z,w) nullptr
#define TracyVkContextCalibrated(x,y,z,w,a,b) nullptr
#define TracyVkDestroy(x)
#define TracyVkContextName(c,x,y)
#define TracyVkNamedZone(c,x,y,z,w)
#define TracyVkNamedZoneC(c,x,y,z,w,a)
#define TracyVkZone(c,x,y)
#define TracyVkZoneC(c,x,y,z)
#define TracyVkZoneTransient(c,x,y,z,w)
#define TracyVkCollect(c,x)

#define TracyVkNamedZoneS(c,x,y,z,w,a)
#define TracyVkNamedZoneCS(c,x,y,z,w,v,a)
#define TracyVkZoneS(c,x,y,z)
#define TracyVkZoneCS(c,x,y,z,w)
#define TracyVkZoneTransientS(c,x,y,z,w,a)

namespace tracy
{
class VkCtxScope {};
}

using TracyVkCtx = void*;

#else

#error Tracy support was not enabled or the Tracy submodule was not initialized.

#endif

#endif
