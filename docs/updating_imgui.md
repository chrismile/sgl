## Updating ImGui

When updating the docking code from ImGui, one might need to add the following check for the use of
`SDL_WINDOW_SKIP_TASKBAR` in imgui_impl_sdl.cpp in order to maintain compatibility with older versions of SDL.

```C++
#if SDL_VERSION_ATLEAST(2, 0, 5)
    // ... code using SDL_WINDOW_SKIP_TASKBAR ...
#endif
```

Also, in order to support multi-viewport windows with the Vulkan rendering backend, the following changed need to be
added to imgui_impl_vulkan.cpp in `ImGui_ImplVulkanH_CreateWindowSwapChain`.

```C++
// We do not create a pipeline by default as this is also used by examples' main.cpp,
// but secondary viewport in multi-viewport mode may want to create one with:
ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
ImGui_ImplVulkan_CreatePipeline(
        device, allocator, VK_NULL_HANDLE, wd->RenderPass, VK_SAMPLE_COUNT_1_BIT, &wd->Pipeline, bd->Subpass);
```

This is due to the following issue: https://github.com/ocornut/imgui/issues/3522
