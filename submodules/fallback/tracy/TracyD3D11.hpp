#ifndef __TRACYD3D11_HPP__
#define __TRACYD3D11_HPP__

#ifndef TRACY_ENABLE

#define TracyD3D11Context(device,queue) nullptr
#define TracyD3D11Destroy(ctx)
#define TracyD3D11ContextName(ctx, name, size)

#define TracyD3D11NewFrame(ctx)

#define TracyD3D11Zone(ctx, name)
#define TracyD3D11ZoneC(ctx, name, color)
#define TracyD3D11NamedZone(ctx, varname, name, active)
#define TracyD3D11NamedZoneC(ctx, varname, name, color, active)
#define TracyD3D12ZoneTransient(ctx, varname, name, active)

#define TracyD3D11ZoneS(ctx, name, depth)
#define TracyD3D11ZoneCS(ctx, name, color, depth)
#define TracyD3D11NamedZoneS(ctx, varname, name, depth, active)
#define TracyD3D11NamedZoneCS(ctx, varname, name, color, depth, active)
#define TracyD3D12ZoneTransientS(ctx, varname, name, depth, active)

#define TracyD3D11Collect(ctx)

namespace tracy
{
class D3D11ZoneScope {};
}

using TracyD3D11Ctx = void*;

#else

#error Tracy support was not enabled or the Tracy submodule was not initialized.

#endif

#endif
