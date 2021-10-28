// Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.  Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#ifndef _VK_RENDER_H_
#define _VK_RENDER_H_

#include <vulkan/vulkan.h>
#include "DX12SharedData.h" // for MAX_SHARED_BUFFERS && AbstractRender

typedef float Vec3[3];
typedef float Vec4[4];
typedef Vec4 Mat4x4[4];

class VkRender : public AbstractRender // SMODE
{
private:
    struct DX12SharedData* m_pSharedData = nullptr;
    PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR = nullptr;
    VkFormat m_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkPhysicalDeviceMemoryProperties m_memoryProperties = { 0, };
    Mat4x4 m_viewProjMatrix = { 0, };

    VkInstance m_inst = nullptr;
    VkDevice m_device = nullptr;
    VkQueue m_queue = nullptr;
    VkCommandPool m_cmdPool = nullptr;
    VkPipelineCache m_pipelineCache = nullptr;
    VkPipeline m_pipeline = nullptr;
    VkRenderPass m_renderPass = nullptr;
    VkPipelineLayout m_pipelineLayout = nullptr;
    VkDescriptorSetLayout m_descLayout = nullptr;
    VkDescriptorPool m_descPool = nullptr;
    VkDescriptorSet m_descSet = nullptr;
    VkImage m_depthImage = nullptr;
    VkDeviceMemory m_depthMem = nullptr;
    VkImageView m_depthView = nullptr;
    VkBuffer m_ubuf = nullptr;
    VkDeviceMemory m_ubufMem = nullptr;
    VkBuffer m_vbuf = nullptr;
    VkDeviceMemory m_vbufMem = nullptr;

    struct _Buffer {
        HANDLE                  sharedMemHandle;
        HANDLE                  sharedFenceHandle;
        VkFramebuffer           framebuffer;
        VkImage                 image;
        VkDeviceMemory          mem;
        VkCommandBuffer         cmd[2];
        VkImageView             view;
        VkSemaphore             semaphore;
        uint64_t                semaphoreFenceValue;
        VkFence                 fence;
        bool                    rendered;
    } m_buffer[MAX_SHARED_BUFFERS] = { 0, };

    uint32_t m_currentBuffer = 0;
    uint32_t m_numFrames = 0;

    bool m_initialized = false;

public:
    VkRender();
    ~VkRender();
    bool Init(DX12SharedData* pSharedData) override;
    void Cleanup() override;
    void Render() override;
    bool Initialized() override  { return m_initialized; }
};

#endif // _VK_RENDER_H_
