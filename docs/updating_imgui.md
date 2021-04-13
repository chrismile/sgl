## Updating ImGui

When updating the docking code from ImGui, one might need to add the following check for the use of
`SDL_WINDOW_SKIP_TASKBAR` in imgui_impl_sdl.cpp in order to maintain compatibility with older versions of SDL.

```C++
#if SDL_VERSION_ATLEAST(2, 0, 5)
    // ... code using SDL_WINDOW_SKIP_TASKBAR ...
#endif
```
