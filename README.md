-------------------------------------------------------------------------------
NVIDIA VkDX12SharedResource
-------------------------------------------------------------------------------

This package contains a sample to demonstrate Vulkan-DX12 interop.
It uses following Vulkan extensions to use DX12 shared memory in Vulkan.
VK_KHR_external_memory_capabilities
VK_KHR_external_memory
VK_KHR_external_memory_win32
VK_KHR_external_semaphore_capabilities
VK_KHR_external_semaphore
VK_KHR_external_semaphore_win32


System requirements:
- Windows 10
- Visual Studio 2015 (any edition) is needed to build the test app
- Vulkan SDK 1.1.70 or later 

Installation:
- Install NVIDIA beta graphics driver 382.83 or later from 
  https://developer.nvidia.com/vulkan-driver.

Usage:

VkDX12SharedResource [options]

Options:
    -mt                Run multi-threaded
    -p                 Run cross-process
    -validate          Validate results
    -vsync             Present after vertical blank
    -dedicated         Use dedicated memory (if supported)
    -n <n>             Use <n> shared buffers (3 <= <n> <= 6)
    -d <n>             Duration in seconds
    -capture <n> <fn>  Capture frame <n> to BMP file <fn>
    -fulltest          Run full QA test
    -h                 Show this help

Known issues
------------
- External memory extensions should not be used with SLI enabled.

Contact Info
------------
Please send any questions, comments or bug reports to vrsupport@nvidia.com.
If submitting a bug report, please note the OS, GPU, driver version, and VR display (HMD).
We strongly encourage including a system log from the DirectX Diagnostic Tool (dxdiag).

Smode dependencies tree
-----------------------

DX12SharedData <- DXPresent  <-DX12SharedResource
               <- VkRender
               <- GLRender



