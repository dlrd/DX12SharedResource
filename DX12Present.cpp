// Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.  Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "DX12Present.h"
#include "stdio.h"
#include "DX12SharedData.h"
#include "d3d12.h"

#define NVIDIA_VENDOR_ID    0x10DE

DX12Present::DX12Present()
{
    ZeroMemory(this, sizeof(DX12Present));
}

DX12Present::~DX12Present()
{
    Cleanup();
}

bool DX12Present::Init(DX12SharedData* pSharedData)
{
    m_pSharedData = pSharedData;

    m_hDC = GetDC(pSharedData->hWnd);

#ifdef _DEBUG
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
        {
            debugController1->SetEnableGPUBasedValidation(true);
        }
        ID3D12Debug5* debugController5 = nullptr;
        if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
        {
            debugController5->SetEnableAutoName(true);
        }
    }
#endif

    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_pFactory));
    if (FAILED(hr))
        return false;

    DXGI_ADAPTER_DESC1 adapterDesc; 
    bool foundNvDevice = false;
    UINT i = 0;
    while (m_pFactory->EnumAdapters1(i, &m_pAdapter) != DXGI_ERROR_NOT_FOUND) { 
        hr = m_pAdapter->GetDesc1(&adapterDesc);
        if (FAILED(hr))
            return false;

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            i++;
            continue;
        }

        //if (adapterDesc.VendorId == NVIDIA_VENDOR_ID)
        {
            foundNvDevice = true;
            m_pSharedData->AdapterLuid = adapterDesc.AdapterLuid;
            break;
        }
        i++;
    } 

    if (!foundNvDevice) {
        fprintf(stderr, "NVIDIA hardware not found.\n");
        return false;
    }

    hr = D3D12CreateDevice(m_pAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_pDevice));
    if (FAILED(hr))
        return false;

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    hr = m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue));
    if (FAILED(hr))
        return false;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = m_pSharedData->numSharedBuffers;
    swapChainDesc.Width = m_pSharedData->width;
    swapChainDesc.Height = m_pSharedData->height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    IDXGISwapChain1* swapChain = NULL;
    hr = m_pFactory->CreateSwapChainForHwnd(m_pCommandQueue, m_pSharedData->hWnd, &swapChainDesc, NULL, NULL, &swapChain);
    if (SUCCEEDED(hr)) {
        hr = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&m_pSwapChain);
    }
    if (swapChain) {
        swapChain->Release();
    }
    if (FAILED(hr))
        return false;

    m_pFactory->MakeWindowAssociation(m_pSharedData->hWnd, DXGI_MWA_NO_ALT_ENTER);

    m_pFactory->Release();
    m_pFactory = nullptr;

    hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator));
    if (FAILED(hr))
        return false;

    for (UINT index = 0; index < m_pSharedData->numSharedBuffers; index++) {
        D3D12_HEAP_PROPERTIES defaultHeapProps = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Width = m_pSharedData->width;
        textureDesc.Height = m_pSharedData->height;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        hr = m_pDevice->CreateCommittedResource(
            &defaultHeapProps,
            D3D12_HEAP_FLAG_SHARED,
            &textureDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            NULL,
            IID_PPV_ARGS(&m_pSharedMem[index]));
        if (FAILED(hr))
            return false;

        hr = m_pDevice->CreateSharedHandle(m_pSharedMem[index], nullptr, GENERIC_ALL, nullptr, &m_sharedMemHandle[index]);
        if (FAILED(hr))
            return false;

        m_pSharedData->sharedMemHandle[index] = m_sharedMemHandle[index];

        hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_pSharedFence[index]));
        if (FAILED(hr))
            return false;

        hr = m_pDevice->CreateSharedHandle(m_pSharedFence[index], nullptr, GENERIC_ALL, nullptr, &m_sharedFenceHandle[index]);
        if (FAILED(hr))
            return false;

        m_pSharedData->sharedFenceHandle[index] = m_sharedFenceHandle[index];

        hr = m_pSwapChain->GetBuffer(index, IID_PPV_ARGS(&m_pRenderTargets[index]));
        if (FAILED(hr))
            return false;

        hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, IID_PPV_ARGS(&m_pCommandList[index]));
        if (FAILED(hr))
            return false;

		D3D12_RESOURCE_BARRIER preCopySrcBarrier = {};
        preCopySrcBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        preCopySrcBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        preCopySrcBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        preCopySrcBarrier.Transition.pResource = m_pSharedMem[index];
        preCopySrcBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        preCopySrcBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
        m_pCommandList[index]->ResourceBarrier(1, &preCopySrcBarrier);

        D3D12_RESOURCE_BARRIER preCopyDstBarrier = {};
        preCopyDstBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        preCopyDstBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        preCopyDstBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        preCopyDstBarrier.Transition.pResource = m_pRenderTargets[index];
        preCopyDstBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        preCopyDstBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        m_pCommandList[index]->ResourceBarrier(1, &preCopyDstBarrier);

        D3D12_TEXTURE_COPY_LOCATION Dst = { m_pRenderTargets[index], D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX };
        D3D12_TEXTURE_COPY_LOCATION Src = { m_pSharedMem[index],    D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX };

        m_pCommandList[index]->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

        D3D12_RESOURCE_BARRIER postCopySrcBarrier = {};
        postCopySrcBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        postCopySrcBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        postCopySrcBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        postCopySrcBarrier.Transition.pResource = m_pSharedMem[index];
        postCopySrcBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
        postCopySrcBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        m_pCommandList[index]->ResourceBarrier(1, &postCopySrcBarrier);

        D3D12_RESOURCE_BARRIER postCopyDstBarrier = {};
        postCopyDstBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        postCopyDstBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        postCopyDstBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        postCopyDstBarrier.Transition.pResource = m_pRenderTargets[index];
        postCopyDstBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        postCopyDstBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        m_pCommandList[index]->ResourceBarrier(1, &postCopyDstBarrier);

        hr = m_pCommandList[index]->Close();
        if (FAILED(hr))
            return false;
    }

    m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    m_pSharedData->currentBufferIndex = m_frameIndex;
    m_initialized = true;
    m_numFrames = 0;
    return true;
}

void DX12Present::Cleanup()
{
    for (UINT i = 0; i < m_pSharedData->numSharedBuffers; i++) {
        if (m_sharedMemHandle[i]) {
            CloseHandle(m_sharedMemHandle[i]);
            m_sharedMemHandle[i] = 0;
        }
        if (m_pSharedMem[i]) {
            m_pSharedMem[i]->Release();
            m_pSharedMem[i] = nullptr;
        }
        if (m_sharedFenceHandle[i]) {
            CloseHandle(m_sharedFenceHandle[i]);
            m_sharedFenceHandle[i] = 0;
        }
        if (m_pSharedFence[i]) {
            m_pSharedFence[i]->Release();
            m_pSharedFence[i] = nullptr;
        }
        if (m_pRenderTargets[i]) {
            m_pRenderTargets[i]->Release();
            m_pRenderTargets[i] = nullptr;
        }
        if (m_pCommandList[i]) {
            m_pCommandList[i]->Release();
            m_pCommandList[i] = nullptr;
        }
    }

    if (m_pCommandQueue) {
        m_pCommandQueue->Release();
        m_pCommandQueue = nullptr;
    }
    if (m_pCommandAllocator) {
        m_pCommandAllocator->Release();
        m_pCommandAllocator = NULL;
    }
    if (m_pSwapChain) {
        m_pSwapChain->Release();
        m_pSwapChain= nullptr;
    }
    if (m_pDevice) {
        m_pDevice->Release();
        m_pDevice = nullptr;
    }
    if (m_pAdapter) {
        m_pAdapter->Release();
        m_pAdapter = nullptr;
    }
    if (m_pFactory) {
        m_pFactory->Release();
        m_pFactory = nullptr;
    }

    if (m_hDC) {
        ReleaseDC(m_pSharedData->hWnd, m_hDC);
        m_hDC = 0;
    }

    m_initialized = false;
}

bool DX12Present::Render()
{
    bool success = true;
    HRESULT hr;

	hr = m_pCommandQueue->Wait(m_pSharedFence[m_frameIndex], ++m_sharedFenceValue[m_frameIndex]);
    if (FAILED(hr))
        return false;

    ID3D12CommandList* ppCommandLists[] = { m_pCommandList[m_frameIndex] };
    m_pCommandQueue->ExecuteCommandLists(ARRAYSIZE(ppCommandLists), ppCommandLists);

    hr = m_pSwapChain->Present(m_pSharedData->vsync ? 1 : 0, 0);
    if (FAILED(hr))
        return false;

    m_pCommandQueue->Signal(m_pSharedFence[m_frameIndex], ++m_sharedFenceValue[m_frameIndex]);

    m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    m_pSharedData->currentBufferIndex = m_frameIndex;
    m_numFrames++;
     
    return success;
}
