// Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.  Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "SmodeErrorAndAssert.h"
#include <vector>
#include "VkRender.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static inline void matrix_multiply(Mat4x4 m, Mat4x4 a, Mat4x4 b)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m[i][j] = 0.f;
            for (int k = 0; k < 4; k++) {
                m[i][j] += a[k][j] * b[i][k];
            }
        }
    }
}

static inline void perspective(Mat4x4 m, float fov, float aspect, float n, float f)
{
    float a = (float)(1.f / tan(fov / 2.f));

    m[0][0] = a / aspect; m[0][1] = 0.f; m[0][2] = 0.f;                        m[0][3] = 0.f;
    m[1][0] = 0.f;        m[1][1] = a;   m[1][2] = 0.f;                        m[1][3] = 0.f;
    m[2][0] = 0.f;        m[2][1] = 0.f; m[2][2] = -((f + n) / (f - n));       m[2][3] = -1.f;
    m[3][0] = 0.f;        m[3][1] = 0.f; m[3][2] = -((2.f * f * n) / (f - n)); m[3][3] = 0.f;
}

static inline void lookAt(Mat4x4 m, Vec3 eye, Vec3 center, Vec3 up)
{
    Vec3 f;
    float p = 0.f;
    for (int i = 0; i < 3; i++) {
        f[i] = center[i] - eye[i];
        p += f[i] * f[i];
    }
    float k = 1.f / sqrtf(p);
    for (int i = 0; i < 3; i++) {
        f[i] *= k;
    }

    Vec3 s;
    s[0] = f[1] * up[2] - f[2] * up[1];
    s[1] = f[2] * up[0] - f[0] * up[2];
    s[2] = f[0] * up[1] - f[1] * up[0];

    p = 0.f;
    for (int i = 0; i < 3; i++) {
        p += s[i] * s[i];
    }
    k = 1.f / sqrtf(p);
    for (int i = 0; i < 3; i++) {
        s[i] *= k;
    }

    Vec3 t;
    t[0] = s[1] * f[2] - s[2] * f[1];
    t[1] = s[2] * f[0] - s[0] * f[2];
    t[2] = s[0] * f[1] - s[1] * f[0];

    m[0][0] = s[0]; m[0][1] = t[0]; m[0][2] = -f[0]; m[0][3] = 0.f;
    m[1][0] = s[1]; m[1][1] = t[1]; m[1][2] = -f[1]; m[1][3] = 0.f;
    m[2][0] = s[2]; m[2][1] = t[2]; m[2][2] = -f[2]; m[2][3] = 0.f;
    m[3][0] = 0.f;  m[3][1] = 0.f;  m[3][2] =  0.f;  m[3][3] = 1.f;

    Vec4 v = { -eye[0], -eye[1], -eye[2], 0};
    for (int i = 0; i < 4; i++) {
        Vec4 r;
        for (int j = 0; j < 4; j++) {
            r[j] = m[j][i];
        }
        p = 0.f;
        for (int j = 0; j < 4; j++) {
            p += r[j] * v[j];
        }
        m[3][i] += p;
    }
}

static inline void mat4x4_y_rotate(Mat4x4 m, float angle)
{
    float s = sinf(angle);
    float c = cosf(angle);

    m[0][0] = c;    m[0][1] = 0.f;  m[0][2] = -s;   m[0][3] = 0.f;
    m[1][0] = 0.f;  m[1][1] = 1.f;  m[1][2] = 0.f;  m[1][3] = 0.f;
    m[2][0] = s;    m[2][1] = 0.f;  m[2][2] = c;    m[2][3] = 0.f;
    m[3][0] = 0.f;  m[3][1] = 0.f;  m[3][2] = 0.f;  m[3][3] = 1.f;
}

static uint32_t getMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& memoryProperties, uint32_t typeBits, VkFlags requirements_mask = 0)
{
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
        if ((typeBits & 1) == 1) {
            if ((memoryProperties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                return i;
            }
        }
        typeBits >>= 1;
    }

    assert(0);
    return VK_MAX_MEMORY_TYPES;
}

static VkShaderModule createShaderModule(VkDevice device, const void *code, size_t size)
{
    VkShaderModule module;
    VkResult err;

    VkShaderModuleCreateInfo shaderModuleCreateInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

    shaderModuleCreateInfo.codeSize = size;
    shaderModuleCreateInfo.pCode = (const uint32_t*)code;
    shaderModuleCreateInfo.flags = 0;
    err = vkCreateShaderModule(device, &shaderModuleCreateInfo, NULL, &module);
    assert(!err);

    return module;
}

VkRender::VkRender()
{
}

VkRender::~VkRender()
{
    Cleanup();
}

bool VkRender::Init(DX12SharedData* pSharedData)
{
    m_pSharedData = pSharedData;

    VkResult err;
    VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    err = vkCreateInstance(&instanceCreateInfo, NULL, &m_inst);
    if (err) {
        fprintf(stderr, "Vulkan: vkCreateInstance failed.\n");
        return false;
    }

    uint32_t gpu_count;
    err = vkEnumeratePhysicalDevices(m_inst, &gpu_count, NULL);
    assert(!err && gpu_count > 0);

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    if (gpu_count > 0) {
        VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * gpu_count);
        err = vkEnumeratePhysicalDevices(m_inst, &gpu_count, physical_devices);
        assert(!err);
        for (uint32_t i = 0; i < gpu_count; i++) {
            VkPhysicalDeviceIDProperties physicalDeviceIDProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES };
            VkPhysicalDeviceProperties2 physicalDeviceProperties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &physicalDeviceIDProperties };

            vkGetPhysicalDeviceProperties2(physical_devices[i], &physicalDeviceProperties2);

            if (!memcmp(physicalDeviceIDProperties.deviceLUID, &m_pSharedData->AdapterLuid, VK_LUID_SIZE)) {
                physicalDevice = physical_devices[i];
                break;
            }
        }
        free(physical_devices);
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        fprintf(stderr, "Vulkan: NVIDIA hardware not found.\n");
        return false;
    }

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memoryProperties);

    VkPhysicalDeviceExternalImageFormatInfo externalImageFormatInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO };
    externalImageFormatInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT;

    VkPhysicalDeviceImageFormatInfo2 imageFormatInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, &externalImageFormatInfo };
    imageFormatInfo.format  = m_format;
    imageFormatInfo.type    = VK_IMAGE_TYPE_2D;
    imageFormatInfo.tiling  = VK_IMAGE_TILING_OPTIMAL;
    imageFormatInfo.usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageFormatInfo.flags   = 0;

    VkExternalImageFormatProperties externalImageFormatProperties = { VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES };
    VkImageFormatProperties2 imageFormatProperties = { VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, &externalImageFormatProperties };

    err = vkGetPhysicalDeviceImageFormatProperties2(physicalDevice, &imageFormatInfo, &imageFormatProperties);
    if ((err == VK_ERROR_FORMAT_NOT_SUPPORTED) ||
        !(externalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT)) {
        fprintf(stderr, "Vulkan: D3D12 memory import not supported.\n");
        return false;
    }
    assert(!err);

    VkPhysicalDeviceExternalSemaphoreInfo externalSemaphoreInfo = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO };
    externalSemaphoreInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT;

    VkExternalSemaphoreProperties externalSemaphoreProperties = { VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES };
    vkGetPhysicalDeviceExternalSemaphoreProperties(physicalDevice, &externalSemaphoreInfo, &externalSemaphoreProperties);

    if (!(externalSemaphoreProperties.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT)) {
        fprintf(stderr, "Vulkan: Import of VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT not supported.\n");
        return false;
    }
    uint32_t enabled_device_extensions = 0;
    const char* required_device_extensions[] = {
        VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
    };

    uint32_t device_extension_count = 0;
    err = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &device_extension_count, NULL);
    assert(!err);

    VkExtensionProperties *device_extensions = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * device_extension_count);
    err = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &device_extension_count, device_extensions);
    assert(!err);

    for (uint32_t i = 0; i < ARRAY_SIZE(required_device_extensions); i++) {
        for (uint32_t j = 0; j < device_extension_count; j++) {
            if (!strcmp(required_device_extensions[i], device_extensions[j].extensionName)) {
                enabled_device_extensions++;
            }
        }
    }

    free(device_extensions);

    if (enabled_device_extensions < ARRAY_SIZE(required_device_extensions)) {
        fprintf(stderr, "Vulkan: Some device extensions are missing.\n");
        return false;
    }

    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
    assert(queueCount >= 1);

    float queuePriorities[1] = { 0.f };
    VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = queuePriorities;

    VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = enabled_device_extensions;
    deviceCreateInfo.ppEnabledExtensionNames = required_device_extensions;

    err = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &m_device);
    if (err) {
        fprintf(stderr, "Vulkan: vkCreateDevice failed.\n");
        return false;
    }

    vkGetDeviceQueue(m_device, 0, 0, &m_queue);

    vkImportSemaphoreWin32HandleKHR = (PFN_vkImportSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(m_device, "vkImportSemaphoreWin32HandleKHR");
    if (vkImportSemaphoreWin32HandleKHR == NULL) {
        fprintf(stderr, "Vulkan: Proc address for \"vkImportSemaphoreWin32HandleKHR\" not found.\n");
        return false;
    }

    VkCommandPoolCreateInfo cmdPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    err = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, NULL, &m_cmdPool);
    assert(!err);

    // define depth buffer
    VkImageCreateInfo depthImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageCreateInfo.format = VK_FORMAT_D16_UNORM;
    depthImageCreateInfo.extent = {(uint32_t)m_pSharedData->width, (uint32_t)m_pSharedData->height, (uint32_t)1};
    depthImageCreateInfo.mipLevels = 1;
    depthImageCreateInfo.arrayLayers = 1;
    depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageCreateInfo.flags = 0;

    VkMemoryRequirements mem_reqs;

    err = vkCreateImage(m_device, &depthImageCreateInfo, NULL, &m_depthImage);
    assert(!err);

    vkGetImageMemoryRequirements(m_device, m_depthImage, &mem_reqs);
    assert(!err);

    VkMemoryAllocateInfo depthMemAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    depthMemAllocInfo.allocationSize = mem_reqs.size;
    depthMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(m_memoryProperties, mem_reqs.memoryTypeBits);

    err = vkAllocateMemory(m_device, &depthMemAllocInfo, NULL, &m_depthMem);
    assert(!err);

    err = vkBindImageMemory(m_device, m_depthImage, m_depthMem, 0);
    assert(!err);

    VkImageViewCreateInfo depthImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    depthImageViewCreateInfo.image = VK_NULL_HANDLE;
    depthImageViewCreateInfo.format = depthImageCreateInfo.format;
    depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    depthImageViewCreateInfo.subresourceRange.levelCount = 1;
    depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    depthImageViewCreateInfo.subresourceRange.layerCount = 1;
    depthImageViewCreateInfo.flags = 0;
    depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthImageViewCreateInfo.image = m_depthImage;
    err = vkCreateImageView(m_device, &depthImageViewCreateInfo, NULL, &m_depthView);
    assert(!err);

    // initialize vertex daza
    VkBufferCreateInfo ubufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    ubufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    ubufCreateInfo.size = sizeof(Mat4x4);
    err = vkCreateBuffer(m_device, &ubufCreateInfo, NULL, &m_ubuf);
    assert(!err);

    vkGetBufferMemoryRequirements(m_device, m_ubuf, &mem_reqs);

    VkMemoryAllocateInfo ubufMemAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    ubufMemAllocInfo.allocationSize = mem_reqs.size;
    ubufMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(m_memoryProperties, mem_reqs.memoryTypeBits);
    err = vkAllocateMemory(m_device, &ubufMemAllocInfo, NULL, &m_ubufMem);
    assert(!err);

    err = vkBindBufferMemory(m_device, m_ubuf, m_ubufMem, 0);
    assert(!err);

    const float verticesAndColors[] = {
        // vertices
        -1.0f,  1.0f,  1.0f,  1.f, 
         1.0f,  1.0f, -1.0f,  1.f,
        -1.0f,  1.0f, -1.0f,  1.f,
         1.0f,  1.0f,  1.0f,  1.f,
         1.0f,  1.0f, -1.0f,  1.f,
        -1.0f,  1.0f,  1.0f,  1.f,

        -1.0f, -1.0f,  1.0f,  1.f,
        -1.0f, -1.0f, -1.0f,  1.f,
         1.0f, -1.0f, -1.0f,  1.f,
         1.0f, -1.0f,  1.0f,  1.f,
        -1.0f, -1.0f,  1.0f,  1.f,
         1.0f, -1.0f, -1.0f,  1.f,

        -1.0f,  1.0f,  1.0f,  1.f,
        -1.0f, -1.0f, -1.0f,  1.f,
        -1.0f, -1.0f,  1.0f,  1.f,
        -1.0f,  1.0f, -1.0f,  1.f,
        -1.0f, -1.0f, -1.0f,  1.f,
        -1.0f,  1.0f,  1.0f,  1.f,

         1.0f,  1.0f,  1.0f,  1.f,
         1.0f, -1.0f,  1.0f,  1.f,
         1.0f, -1.0f, -1.0f,  1.f,
         1.0f,  1.0f, -1.0f,  1.f,
         1.0f,  1.0f,  1.0f,  1.f,
         1.0f, -1.0f, -1.0f,  1.f,

        -1.0f,  1.0f, -1.0f,  1.f,
         1.0f, -1.0f, -1.0f,  1.f,
        -1.0f, -1.0f, -1.0f,  1.f,
         1.0f,  1.0f, -1.0f,  1.f,
         1.0f, -1.0f, -1.0f,  1.f,
        -1.0f,  1.0f, -1.0f,  1.f,

        -1.0f,  1.0f,  1.0f,  1.f,
        -1.0f, -1.0f,  1.0f,  1.f,
         1.0f, -1.0f,  1.0f,  1.f,
         1.0f,  1.0f,  1.0f,  1.f,
        -1.0f,  1.0f,  1.0f,  1.f,
         1.0f, -1.0f,  1.0f,  1.f,

        // colors
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 1.0f,
    };

    VkBufferCreateInfo vbufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    vbufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    vbufCreateInfo.size = sizeof(verticesAndColors);
    err = vkCreateBuffer(m_device, &vbufCreateInfo, NULL, &m_vbuf);
    assert(!err);

    vkGetBufferMemoryRequirements(m_device, m_vbuf, &mem_reqs);

    VkMemoryAllocateInfo vbufMemAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    vbufMemAllocInfo.allocationSize = mem_reqs.size;
    vbufMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(m_memoryProperties, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    err = vkAllocateMemory(m_device, &vbufMemAllocInfo, NULL, &(m_vbufMem));
    assert(!err);

    uint8_t* pData;
    err = vkMapMemory(m_device, m_vbufMem, 0, vbufMemAllocInfo.allocationSize, 0, (void**)&pData);
    assert(!err);
    memcpy(pData, verticesAndColors, sizeof(verticesAndColors));
    vkUnmapMemory(m_device, m_vbufMem);

    err = vkBindBufferMemory(m_device, m_vbuf, m_vbufMem, 0);
    assert(!err);

    // initialize descriptor layout
    VkDescriptorSetLayoutBinding bindings[2];
    memset(&bindings, 0, sizeof(bindings));
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[0].pImmutableSamplers = NULL;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[1].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetLayoutCreateInfo.bindingCount = 2;
    descriptorSetLayoutCreateInfo.pBindings = bindings;
    err = vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, NULL, &m_descLayout);
    assert(!err);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_descLayout;
    err = vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, NULL, &m_pipelineLayout);
    assert(!err);

    // initialize render pass
    VkAttachmentDescription attachmentDesc[2];
    memset(attachmentDesc, 0, sizeof(attachmentDesc));
    attachmentDesc[0].format = m_format;
    attachmentDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDesc[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDesc[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachmentDesc[1].format = depthImageCreateInfo.format;
    attachmentDesc[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDesc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDesc[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDesc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDesc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDesc[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentDesc[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_reference;
    memset(&color_reference, 0, sizeof(color_reference));
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_reference;
    memset(&depth_reference, 0, sizeof(depth_reference));
    depth_reference.attachment = 1;
    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass;
    memset(&subpass, 0, sizeof(subpass));
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = NULL;
    subpass.pDepthStencilAttachment = &depth_reference;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassCreateInfo.attachmentCount = 2;
    renderPassCreateInfo.pAttachments = attachmentDesc;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 0;
    renderPassCreateInfo.pDependencies = NULL;

    err = vkCreateRenderPass(m_device, &renderPassCreateInfo, NULL, &m_renderPass);
    assert(!err);

    // initialize pipeline
    std::vector<VkDynamicState> dynamicStateEnables;


    VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };


    VkGraphicsPipelineCreateInfo pipeline = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipeline.layout = m_pipelineLayout;

    VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo rasterState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterState.depthClampEnable = VK_FALSE;
    rasterState.rasterizerDiscardEnable = VK_FALSE;
    rasterState.depthBiasEnable = VK_FALSE;
    rasterState.lineWidth = 1.0f;

    VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState[1];
    memset(colorBlendAttachmentState, 0, sizeof(colorBlendAttachmentState));
    colorBlendAttachmentState[0].colorWriteMask = 0xf;
    colorBlendAttachmentState[0].blendEnable = VK_FALSE;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = colorBlendAttachmentState;

    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());

    VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
    depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = depthStencilState.back;

    VkPipelineMultisampleStateCreateInfo multiSampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multiSampleState.pSampleMask = NULL;
    multiSampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // create shader
    const char vertShaderCode[] = 
        "#version 450\n"
        "\n"
        "layout(binding = 0) uniform _ubuf {\n"
        "    mat4 MVP;\n"
        "} ubuf;\n"
        "\n"
        "layout(binding = 1) uniform _vbuf {\n"
        "    vec4 position[12 * 3];\n"
        "    vec4 color[6];\n"
        "} vbuf;\n"
        "\n"
        "layout (location = 0) out vec4 color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   color = vbuf.color[gl_VertexIndex / 6];\n"
        "   gl_Position = ubuf.MVP * vbuf.position[gl_VertexIndex];\n"
        "\n"
        "   // GL->VK conventions\n"
        "   gl_Position.y = -gl_Position.y;\n"
        "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
        "}\n";

    VkShaderModule vs = createShaderModule(m_device, vertShaderCode, sizeof(vertShaderCode));

    const char fragShaderCode[] = 
        "#version 450\n"
        "\n"
        "layout (location = 0) in vec4 color;\n"
        "layout (location = 0) out vec4 uFragColor;\n"
        "\n"
        "void main() \n"
        "{\n"
        "    uFragColor = color;\n"
        "}\n";
    VkShaderModule fs = createShaderModule(m_device, fragShaderCode, sizeof(fragShaderCode));

    pipeline.stageCount = 2;
    VkPipelineShaderStageCreateInfo shaderStages[2] = { { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO }, 
                                                        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO } };

    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vs;
    shaderStages[0].pName = "main";

    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fs;
    shaderStages[1].pName = "main";

    VkPipelineCacheCreateInfo pipelineCache = { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };

    err = vkCreatePipelineCache(m_device, &pipelineCache, NULL, &m_pipelineCache);
    assert(!err);

    pipeline.pVertexInputState = &vertexInputState;
    pipeline.pInputAssemblyState = &inputAssemblyState;
    pipeline.pRasterizationState = &rasterState;
    pipeline.pColorBlendState = &colorBlendState;
    pipeline.pMultisampleState = &multiSampleState;
    pipeline.pViewportState = &viewportState;
    pipeline.pDepthStencilState = &depthStencilState;
    pipeline.pStages = shaderStages;
    pipeline.renderPass = m_renderPass;
    pipeline.pDynamicState = &dynamicState;

    pipeline.renderPass = m_renderPass;

    err = vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipeline, NULL, &m_pipeline);
    assert(!err);

    vkDestroyShaderModule(m_device, vs, NULL);
    vkDestroyShaderModule(m_device, fs, NULL);

    // initialize descriptor set
    VkDescriptorPoolSize type_count;
    memset(&type_count, 0, sizeof(type_count));
    type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    descriptorPoolCreateInfo.maxSets = 1;
    descriptorPoolCreateInfo.poolSizeCount = 1;
    descriptorPoolCreateInfo.pPoolSizes = &type_count;

    err = vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, NULL, &m_descPool);
    assert(!err);

    VkDescriptorSetAllocateInfo descriptorSetAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    descriptorSetAllocInfo.descriptorPool = m_descPool;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    descriptorSetAllocInfo.pSetLayouts = &m_descLayout;
    err = vkAllocateDescriptorSets(m_device, &descriptorSetAllocInfo, &m_descSet);
    assert(!err);

    VkWriteDescriptorSet descriptorWrites[] = { { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET }, 
                                                { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET } };

    VkDescriptorBufferInfo descBufferInfo[] = { { m_ubuf, 0, sizeof(Mat4x4) }, { m_vbuf, 0, sizeof(verticesAndColors) } };

    descriptorWrites[0].dstSet = m_descSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].pBufferInfo = &descBufferInfo[0];

    descriptorWrites[1].dstSet = m_descSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[1].pBufferInfo = &descBufferInfo[1];

    vkUpdateDescriptorSets(m_device, 2, descriptorWrites, 0, NULL);

    bool useDedicatedMemory = (m_pSharedData->forceDedicatedMemory || 
                               (externalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT));

    for (uint32_t i = 0; i < m_pSharedData->numSharedBuffers; i++) {
        VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        err = vkCreateFence(m_device, &fenceInfo, NULL, &m_buffer[i].fence);
        assert(!err);

        m_buffer[i].sharedFenceHandle = pSharedData->sharedFenceHandle[i];

        VkSemaphoreCreateInfo semCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        err = vkCreateSemaphore(m_device, &semCreateInfo, NULL, &m_buffer[i].semaphore);
        assert(!err);

        VkImportSemaphoreWin32HandleInfoKHR importSemWin32Info = { VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR };
        importSemWin32Info.semaphore = m_buffer[i].semaphore;
        importSemWin32Info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT;
        importSemWin32Info.handle = m_buffer[i].sharedFenceHandle;
        err = vkImportSemaphoreWin32HandleKHR(m_device, &importSemWin32Info);
        assert(!err);

        m_buffer[i].semaphoreFenceValue = 0;

        m_buffer[i].sharedMemHandle = pSharedData->sharedMemHandle[i];

        VkCommandBufferAllocateInfo cmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        cmdAllocInfo.commandPool = m_cmdPool;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = 2;

        err = vkAllocateCommandBuffers(m_device, &cmdAllocInfo, m_buffer[i].cmd);
        assert(!err);

        VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo = { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO };
        externalMemoryImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT;

        VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, &externalMemoryImageCreateInfo };
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo.extent.width = m_pSharedData->width;
        imageCreateInfo.extent.height = m_pSharedData->height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        imageCreateInfo.flags = 0;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        err = vkCreateImage(m_device, &imageCreateInfo, NULL, &m_buffer[i].image);
        assert(!err);

        VkMemoryRequirements memReqs = { };
        vkGetImageMemoryRequirements(m_device, m_buffer[i].image, &memReqs);

        uint32_t memoryTypeIndex = getMemoryTypeIndex(m_memoryProperties, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memoryTypeIndex >= m_memoryProperties.memoryTypeCount) {
            fprintf(stderr, "Vulkan: Memory doesn't support sharing.\n");
            return false;
        }

        VkImportMemoryWin32HandleInfoKHR importMemInfo = { VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
        importMemInfo.handleType = (VkExternalMemoryHandleTypeFlagBits)externalMemoryImageCreateInfo.handleTypes;
        importMemInfo.handle = m_buffer[i].sharedMemHandle;

        VkMemoryAllocateInfo memInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &importMemInfo };
        memInfo.allocationSize = memReqs.size;
        memInfo.memoryTypeIndex = memoryTypeIndex;

        VkMemoryDedicatedAllocateInfo memoryDedicatedAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
        if (useDedicatedMemory) {
            memoryDedicatedAllocateInfo.image = m_buffer[i].image;
            importMemInfo.pNext = &memoryDedicatedAllocateInfo;
        }

        err = vkAllocateMemory(m_device, &memInfo, 0, &m_buffer[i].mem);
        assert(!err);

        err = vkBindImageMemory(m_device, m_buffer[i].image, m_buffer[i].mem, 0);
        assert(!err);

        VkImageViewCreateInfo imageView = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageView.image = m_buffer[i].image;
        imageView.format = m_format;
        imageView.components.r = VK_COMPONENT_SWIZZLE_R;
        imageView.components.g = VK_COMPONENT_SWIZZLE_G;
        imageView.components.b = VK_COMPONENT_SWIZZLE_B;
        imageView.components.a = VK_COMPONENT_SWIZZLE_A;
        imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageView.subresourceRange.baseMipLevel = 0;
        imageView.subresourceRange.levelCount = 1;
        imageView.subresourceRange.baseArrayLayer = 0;
        imageView.subresourceRange.layerCount = 1;
        imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageView.flags = 0;

        err = vkCreateImageView(m_device, &imageView, NULL, &m_buffer[i].view);
        assert(!err);

        VkImageView attachments[2];
        attachments[1] = m_depthView;

        VkFramebufferCreateInfo frameBufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        frameBufferCreateInfo.renderPass = m_renderPass;
        frameBufferCreateInfo.attachmentCount = 2;
        frameBufferCreateInfo.pAttachments = attachments;
        frameBufferCreateInfo.width = m_pSharedData->width;
        frameBufferCreateInfo.height = m_pSharedData->height;
        frameBufferCreateInfo.layers = 1;

        attachments[0] = m_buffer[i].view;
        err = vkCreateFramebuffer(m_device, &frameBufferCreateInfo, NULL, &m_buffer[i].framebuffer);
        assert(!err);

        VkCommandBuffer cmd = m_buffer[i].cmd[1];

        VkCommandBufferInheritanceInfo cmdInheritanceInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
        cmdInheritanceInfo.renderPass = VK_NULL_HANDLE;
        cmdInheritanceInfo.subpass = 0;
        cmdInheritanceInfo.framebuffer = VK_NULL_HANDLE;
        cmdInheritanceInfo.occlusionQueryEnable = VK_FALSE;
        cmdInheritanceInfo.queryFlags = 0;
        cmdInheritanceInfo.pipelineStatistics = 0;

        VkCommandBufferBeginInfo cmdBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        cmdBeginInfo.flags = 0;
        cmdBeginInfo.pInheritanceInfo = &cmdInheritanceInfo;

        VkClearValue clearValues[2];
        memset(clearValues, 0, sizeof(clearValues));
        clearValues[0].color.float32[0] = 0.1f;
        clearValues[0].color.float32[1] = 0.1f;
        clearValues[0].color.float32[2] = 0.4f;
        clearValues[0].color.float32[3] = 1.0f;
        clearValues[1].depthStencil.depth = 1.0f;
        clearValues[1].depthStencil.stencil = 0;

        VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassBeginInfo.renderPass = m_renderPass;
        renderPassBeginInfo.framebuffer = m_buffer[i].framebuffer;
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent.width = m_pSharedData->width;
        renderPassBeginInfo.renderArea.extent.height = m_pSharedData->height;
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;

        err = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
        assert(!err);

        vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descSet, 0, NULL);

        VkViewport viewport;
        memset(&viewport, 0, sizeof(viewport));
        viewport.width = (float)m_pSharedData->width;
        viewport.height = (float)m_pSharedData->height;
        viewport.minDepth = (float)0.0f;
        viewport.maxDepth = (float)1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor;
        memset(&scissor, 0, sizeof(scissor));
        scissor.extent.width = m_pSharedData->width;
        scissor.extent.height = m_pSharedData->height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdDraw(cmd, 12 * 3, 1, 0, 0);
        vkCmdEndRenderPass(cmd);

        err = vkEndCommandBuffer(cmd);
        assert(!err);

        m_buffer[i].rendered = false;
    }

    Vec3 eyePos = { 0.f, 3.f, -6.f };
    Vec3 origin = { 0.f, 0.5f, 0.f };
    Vec3 upVector = { 0.f, 1.f, 0.f };

    Mat4x4 projMatrix, viewMatrix;
    perspective(projMatrix, (float)(45.f * M_PI / 180.f), (float)m_pSharedData->width / (float)m_pSharedData->height, 0.1f, 100.0f);
    lookAt(viewMatrix, eyePos, origin, upVector);
    matrix_multiply(m_viewProjMatrix, projMatrix, viewMatrix);

    m_currentBuffer = 0;
    m_numFrames = 0;

    m_initialized = true;

    return true;
}

void VkRender::Cleanup() 
{
    m_initialized = false;

    if (m_device) {
        vkDeviceWaitIdle(m_device);

        if (m_descPool) {
            vkDestroyDescriptorPool(m_device, m_descPool, NULL);
            m_descPool = 0;
        }

        if (m_pipeline) {
            vkDestroyPipeline(m_device, m_pipeline, NULL);
            m_pipeline = 0;
        }

        if (m_pipelineCache) {
            vkDestroyPipelineCache(m_device, m_pipelineCache, NULL);
            m_pipelineCache = 0;
        }

        if (m_renderPass) {
            vkDestroyRenderPass(m_device, m_renderPass, NULL);
            m_renderPass = 0;
        }

        if (m_pipelineLayout) {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, NULL);
            m_pipelineLayout = 0;
        }

        if (m_descLayout) {
            vkDestroyDescriptorSetLayout(m_device, m_descLayout, NULL);
            m_descLayout = 0;
        }

        for (uint32_t i = 0; i < m_pSharedData->numSharedBuffers; i++) {
            if (m_buffer[i].framebuffer) {
                vkDestroyFramebuffer(m_device, m_buffer[i].framebuffer, NULL);
                m_buffer[i].framebuffer = 0;
            }
            if (m_buffer[i].fence) {
                vkDestroyFence(m_device, m_buffer[i].fence, NULL);
                m_buffer[i].fence = 0;
            }
            if (m_buffer[i].semaphore) {
                vkDestroySemaphore(m_device, m_buffer[i].semaphore, NULL);
                m_buffer[i].semaphore = 0;
            }
            if (m_buffer[i].image) {
                vkDestroyImage(m_device, m_buffer[i].image, NULL);
                m_buffer[i].image = 0;
            }
            if (m_buffer[i].view) {
                vkDestroyImageView(m_device, m_buffer[i].view, NULL);
                m_buffer[i].view = 0;
            }
            if (m_buffer[i].mem) {
                vkFreeMemory(m_device, m_buffer[i].mem, NULL);
                m_buffer[i].mem = 0;
            }
            if (m_buffer[i].cmd[0]) {
                vkFreeCommandBuffers(m_device, m_cmdPool, 1, &m_buffer[i].cmd[0]);
                m_buffer[i].cmd[0] = 0;
            }
            if (m_buffer[i].cmd[1]) {
                vkFreeCommandBuffers(m_device, m_cmdPool, 1, &m_buffer[i].cmd[1]);
                m_buffer[i].cmd[1] = 0;
            }
        }

        if (m_depthView) {
            vkDestroyImageView(m_device, m_depthView, NULL);
            m_depthView = 0;
        }

        if (m_depthImage) {
            vkDestroyImage(m_device, m_depthImage, NULL);
            m_depthImage = 0;
        }

        if (m_depthMem) {
            vkFreeMemory(m_device, m_depthMem, NULL);
            m_depthMem = 0;
        }

        if (m_ubuf) {
            vkDestroyBuffer(m_device, m_ubuf, NULL);
            m_ubuf = 0;
        }

        if (m_ubufMem) {
            vkFreeMemory(m_device, m_ubufMem, NULL);
            m_ubufMem = 0;
        }

        if (m_vbuf) {
            vkDestroyBuffer(m_device, m_vbuf, NULL);
            m_vbuf = 0;
        }

        if (m_vbufMem) {
            vkFreeMemory(m_device, m_vbufMem, NULL);
            m_vbufMem = 0;
        }

        if (m_cmdPool) {
            vkDestroyCommandPool(m_device, m_cmdPool, NULL);
            m_cmdPool = 0;
        }

        vkDestroyDevice(m_device, NULL);
        m_device = 0;
    }

    if (m_inst) {
        vkDestroyInstance(m_inst, NULL);
        m_inst = 0;
    }
}

void VkRender::Render() 
{
    VkResult err = VK_SUCCESS;

    if (!m_initialized)
        return;

    m_currentBuffer = m_pSharedData->currentBufferIndex;

    float angle;
    /*if (m_pSharedData->verify) {
        angle = (float)M_PI * (m_numFrames % 4) / 2.f;
    } else if (m_pSharedData->captureFile) {
        angle = (float)M_PI * (m_numFrames % 1000) / 500.f;
    } else*/ {
        static ULONGLONG timeStart = 0;
        ULONGLONG timeCur = GetTickCount64();
        if (timeStart == 0)
            timeStart = timeCur;
        angle = (timeCur - timeStart) / 1000.0f;
    }

    Mat4x4 modelViewProjMatrix;
    Mat4x4 modelMatrix;
    mat4x4_y_rotate(modelMatrix, angle);
    matrix_multiply(modelViewProjMatrix, m_viewProjMatrix, modelMatrix);

    if (m_buffer[m_currentBuffer].rendered) {
        vkWaitForFences(m_device, 1, &m_buffer[m_currentBuffer].fence, VK_TRUE, 0xFFFFFFFFFFFFFFFFULL);
    }
    err = vkResetCommandBuffer(m_buffer[m_currentBuffer].cmd[0], 0);
    assert(!err);

    VkCommandBufferBeginInfo cmdBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    err = vkBeginCommandBuffer(m_buffer[m_currentBuffer].cmd[0], &cmdBeginInfo);
    assert(!err);

    vkCmdUpdateBuffer(m_buffer[m_currentBuffer].cmd[0], m_ubuf, 0, sizeof(modelViewProjMatrix), reinterpret_cast<uint32_t const *>(&modelViewProjMatrix[0][0]));

    err = vkEndCommandBuffer(m_buffer[m_currentBuffer].cmd[0]);
    assert(!err);

    const UINT64 waitFence = ((UINT64)m_buffer[m_currentBuffer].semaphoreFenceValue);
    const UINT64 signalFence = waitFence + 1;

    m_buffer[m_currentBuffer].semaphoreFenceValue += 2;


    VkD3D12FenceSubmitInfoKHR fenceSubmitInfo = { VK_STRUCTURE_TYPE_D3D12_FENCE_SUBMIT_INFO_KHR };
    fenceSubmitInfo.waitSemaphoreValuesCount = 1;
    fenceSubmitInfo.pWaitSemaphoreValues = &waitFence;
    fenceSubmitInfo.signalSemaphoreValuesCount = 1;
    fenceSubmitInfo.pSignalSemaphoreValues = &signalFence; 

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO, &fenceSubmitInfo };
    submit_info.commandBufferCount = 2;
    submit_info.pCommandBuffers = m_buffer[m_currentBuffer].cmd;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &m_buffer[m_currentBuffer].semaphore;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &m_buffer[m_currentBuffer].semaphore;

    err = vkQueueSubmit(m_queue, 1, &submit_info, m_buffer[m_currentBuffer].fence);
    assert(!err);

    m_buffer[m_currentBuffer].rendered = true;

    m_numFrames++;
}

//AbstractRender* newAbstractRender()
//{
//  return new VkRender();
//}