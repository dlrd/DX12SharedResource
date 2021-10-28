#\-------------------------------------- . -----------------------------------/#
# Filename : CMakeList.txt               | CMakeLists file for OpenGL          #
# Author   : Alexandre Buge              |  Smode Player                       #
# Started  : 02/10/2016 12:03            |                                     #
#/-------------------------------------- . -----------------------------------\#

PROJECT(DX12SharedResource CXX C)
cmake_minimum_required(VERSION 3.18)

FIND_PACKAGE(OpenGL REQUIRED)
SET (Vulkan_SDK_PATH $ENV{VULKAN_SDK})

add_executable(DX12SharedResource 
  DX12Present.cpp 
  DX12Present.h
  DX12SharedData.h
  DX12SharedResource.cpp
  GLExtensions.h
  WGLExtensions.h
  GLRender.h
  GLRender.cpp
  README.md
  VkRender.cpp
  VkRender.h
  SmodeErrorAndAssert.h
  SmodeErrorAndAssert.cpp
)

target_include_directories(DX12SharedResource PRIVATE ${Vulkan_SDK_PATH}/Include ${OPENGL_INCLUDE_DIR})
target_compile_definitions(DX12SharedResource PRIVATE _UNICODE UNICODE _USE_MATH_DEFINES VK_USE_PLATFORM_WIN32_KHR)

target_compile_definitions(DX12SharedResource PRIVATE MAX_SHARED_BUFFERS=4)

target_link_directories(DX12SharedResource PRIVATE ${Vulkan_SDK_PATH}/Lib)
target_link_libraries(DX12SharedResource PRIVATE vulkan-1 d3d12 dxgi  	Opengl32 ${OPENGL_LIBRARIES})

SET_PROPERTY(DIRECTORY ${Smode_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "DX12SharedResource")
 #oil_configure_extern_application(DX12SharedResource)
 