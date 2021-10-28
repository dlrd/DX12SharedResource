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

        if (adapterDesc.VendorId == NVIDIA_VENDOR_ID) {
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

    //m_readbackFenceValue = 0;

    //hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pReadbackFence));
    //if (FAILED(hr))
    //    return false;

    //m_readbackFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    //if (!m_readbackFenceEvent)
    //    return false;

    //D3D12_HEAP_PROPERTIES readbackHeapProps = { D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

    //D3D12_RESOURCE_DESC descReadbackTex = { };
    //descReadbackTex.MipLevels = 1;
    //descReadbackTex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    //if (m_pSharedData->captureFile) {
    //    descReadbackTex.Width = m_pSharedData->width;
    //    descReadbackTex.Height = m_pSharedData->height;
    //else 
    //{
    //    descReadbackTex.Width = 4;
    //    descReadbackTex.Height = 4;
    //}
    //descReadbackTex.Flags = D3D12_RESOURCE_FLAG_NONE;
    //descReadbackTex.DepthOrArraySize = 1;
    //descReadbackTex.SampleDesc.Count = 1;
    //descReadbackTex.SampleDesc.Quality = 0;
    //descReadbackTex.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    //descReadbackTex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    //D3D12_RESOURCE_DESC readbackBufferDesc = { };
    //readbackBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    //readbackBufferDesc.Height = 1;
    //readbackBufferDesc.DepthOrArraySize = 1;
    //readbackBufferDesc.MipLevels = 1;
    //readbackBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    //readbackBufferDesc.SampleDesc.Count = 1;
    //readbackBufferDesc.SampleDesc.Quality = 0;
    //readbackBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    //readbackBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    //m_pReadbackTexLayout = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(malloc(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT)));
    //if (!m_pReadbackTexLayout)
    //    return false;
    //
    //m_pDevice->GetCopyableFootprints(&descReadbackTex, 0, 1, 0, m_pReadbackTexLayout, NULL, NULL, &readbackBufferDesc.Width);

    //hr = m_pDevice->CreateCommittedResource(
    //    &readbackHeapProps,
    //    D3D12_HEAP_FLAG_NONE,
    //    &readbackBufferDesc,
    //    D3D12_RESOURCE_STATE_COPY_DEST,
    //    NULL,
    //    IID_PPV_ARGS(&m_pReadback));
    //if (FAILED(hr))
    //    return false;

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

        D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE };
        barrier.Transition.pResource   = m_pSharedMem[index];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_pCommandList[index]->ResourceBarrier(1, &barrier);

        D3D12_TEXTURE_COPY_LOCATION Dst = { m_pRenderTargets[index], D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX };
        D3D12_TEXTURE_COPY_LOCATION Src = { m_pSharedMem[index],    D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX };

        m_pCommandList[index]->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);

        hr = m_pCommandList[index]->Close();
        if (FAILED(hr))
            return false;

        //hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, IID_PPV_ARGS(&m_pReadbackCommandList[index]));
        //if (FAILED(hr))
        //    return false;

        //D3D12_RESOURCE_BARRIER readbackBarrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE };
        //readbackBarrier.Transition.pResource   = m_pSharedMem[index];
        //readbackBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        //readbackBarrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
        //readbackBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        //m_pReadbackCommandList[index]->ResourceBarrier(1, &readbackBarrier);

        //D3D12_TEXTURE_COPY_LOCATION readbackDst = { m_pReadback, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, *m_pReadbackTexLayout };
        //D3D12_TEXTURE_COPY_LOCATION readbackSrc = { m_pSharedMem[index], D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX };
        D3D12_BOX box;
 /*       if (m_pSharedData->captureFile) {
            box.left   = 0;
            box.top    = 0;
            box.right  = m_pSharedData->width;
            box.bottom = m_pSharedData->height;
        } else */
        {
            box.left   = m_pSharedData->width / 2;
            box.top    = m_pSharedData->height * 3 / 4;
            box.right  = box.left + 4;
            box.bottom = box.top  + 4;
        }
        box.front  = 0;
        box.back   = 1;

        //m_pReadbackCommandList[index]->CopyTextureRegion(&readbackDst, 0, 0, 0, &readbackSrc, &box);

        //hr = m_pReadbackCommandList[index]->Close();
        //if (FAILED(hr))
        //    return false;

    }

    m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    m_pSharedData->currentBufferIndex = m_frameIndex;
    m_initialized = true;
    m_numFrames = 0;
    return true;
}

void DX12Present::Cleanup()
{
    //WaitForCompletion();

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
        //if (m_pReadbackCommandList[i]) {
        //    m_pReadbackCommandList[i]->Release();
        //    m_pReadbackCommandList[i] = nullptr;
        //}
        if (m_pCommandList[i]) {
            m_pCommandList[i]->Release();
            m_pCommandList[i] = nullptr;
        }
    }

    //if (m_pReadback) {
    //    m_pReadback->Release();
    //    m_pReadback = nullptr;
    //}
    //if (m_pReadbackTexLayout) {
    //    free(m_pReadbackTexLayout);
    //    m_pReadbackTexLayout = nullptr;
    //}
    //if (m_readbackFenceEvent) {
    //    CloseHandle(m_readbackFenceEvent);
    //    m_readbackFenceEvent = nullptr;
    //}
    //if (m_pReadbackFence) {
    //    m_pReadbackFence->Release();
    //    m_pReadbackFence = nullptr;
    //}
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
//
//bool DX12Present::VerifyResult()
//{
//    HRESULT hr;
//
//    ID3D12CommandList* ppReadbackCommandLists[] = { m_pReadbackCommandList[m_frameIndex] };
//    m_pCommandQueue->ExecuteCommandLists(ARRAYSIZE(ppReadbackCommandLists), ppReadbackCommandLists);
//        
//    hr = m_pCommandQueue->Signal(m_pReadbackFence, ++m_readbackFenceValue);
//    if (FAILED(hr))
//        return false;
//
//    if (m_pReadbackFence->GetCompletedValue() < m_readbackFenceValue) {
//        hr = m_pReadbackFence->SetEventOnCompletion(m_readbackFenceValue, m_readbackFenceEvent);
//        if (FAILED(hr))
//            return false;
//        WaitForSingleObject(m_readbackFenceEvent, INFINITE);
//    }
//
//    //                        cyan,       yellow,     magenta,    blue
//    const UINT expected[] = { 0xffffff00, 0xff00ffff, 0xffff00ff, 0xffff0000 };
//
//    void* pData;
//    hr = m_pReadback->Map(0, NULL, reinterpret_cast<void**>(&pData));
//    if (SUCCEEDED(hr)) {
//        for (UINT y = 0; y < 4; y++) {
//            PBYTE p = reinterpret_cast<BYTE*>(pData) + m_pReadbackTexLayout->Footprint.RowPitch * y;
//            for (UINT x = 0; x < 4; x++) {
//                if (*((UINT*)p) != expected[m_numFrames % 4]) {
//                    m_pReadback->Unmap(0, NULL);
//                    assert(SUCCEEDED(hr));
//                    return false;
//                }
//                p += 4;
//            }
//        }
//    }
//
//    m_pReadback->Unmap(0, NULL);
//
//    return true;
//};
//
//bool DX12Present::CaptureFrame()
//{
//    HRESULT hr;
//
//    ID3D12CommandList* ppReadbackCommandLists[] = { m_pReadbackCommandList[m_frameIndex] };
//    m_pCommandQueue->ExecuteCommandLists(ARRAYSIZE(ppReadbackCommandLists), ppReadbackCommandLists);
//        
//    hr = m_pCommandQueue->Signal(m_pReadbackFence, ++m_readbackFenceValue);
//    if (FAILED(hr))
//        return false;
//
//    if (m_pReadbackFence->GetCompletedValue() < m_readbackFenceValue) {
//        hr = m_pReadbackFence->SetEventOnCompletion(m_readbackFenceValue, m_readbackFenceEvent);
//        if (FAILED(hr))
//            return false;
//        WaitForSingleObject(m_readbackFenceEvent, INFINITE);
//    }
//
//    void* pData;
//    hr = m_pReadback->Map(0, NULL, reinterpret_cast<void**>(&pData));
//    if (SUCCEEDED(hr)) {
//        WriteBMP(m_pSharedData->captureFile, (LPCBYTE)pData, m_pSharedData->width, m_pReadbackTexLayout->Footprint.RowPitch, m_pSharedData->height);
//    }
//
//    m_pReadback->Unmap(0, NULL);
//
//    return true;
//};

bool DX12Present::Render()
{
    bool success = true;
    HRESULT hr;

    hr = m_pCommandQueue->Wait(m_pSharedFence[m_frameIndex], ++m_sharedFenceValue[m_frameIndex]);
    if (FAILED(hr))
        return false;

    //if (m_pSharedData->captureFile) {
    //    if (m_pSharedData->captureFrame == m_numFrames) {
    //        CaptureFrame();
    //    }
    //} else if (m_pSharedData->verify && ((m_numFrames % 8) == 0)) {
    //    success = VerifyResult();
    //}

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

//void DX12Present::WaitForCompletion()
//{
//    if (m_pReadbackFence && m_readbackFenceEvent) {
//        HRESULT hr = m_pCommandQueue->Signal(m_pReadbackFence, ++m_readbackFenceValue);
//        assert(SUCCEEDED(hr));
//
//        if (m_pReadbackFence->GetCompletedValue() < m_readbackFenceValue) {
//            hr = m_pReadbackFence->SetEventOnCompletion(m_readbackFenceValue, m_readbackFenceEvent);
//            assert(SUCCEEDED(hr));
//            WaitForSingleObject(m_readbackFenceEvent, INFINITE);
//        }
//    }
//}
//
//bool DX12Present::WriteBMP(LPCSTR lpszFilename, LPCBYTE pPixels, UINT width, UINT rowPitch, UINT height)
//{
//    FILE* fp = NULL;
//    if (fopen_s(&fp, lpszFilename, "wb")) {
//        return false;
//    }
//
//    BITMAPFILEHEADER bfh = {
//        0x4d42, // "BM"
//        0, 0, 0,
//        sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER),
//    };
//    BITMAPINFOHEADER bih =
//    {
//        sizeof(BITMAPINFOHEADER),
//        (LONG)width, -((LONG)height),   // Negative height makes it go top-down
//        1, 32, BI_RGB,
//    };
//
//    fwrite(&bfh, sizeof(bfh), 1, fp);
//    fwrite(&bih, sizeof(bih), 1, fp);
//
//    for (UINT y = 0; y < height; y++) {
//        LPCBYTE pPixel = pPixels + y * rowPitch;
//        for (UINT x = 0; x < width; x++) {
//            BYTE bgra[4] = { pPixel[2], pPixel[1], pPixel[0], pPixel[3] };
//            fwrite(&bgra, 4, 1, fp);
//            pPixel += 4;
//        }
//    }
//
//    fclose(fp);
//
//    return true;
//}
