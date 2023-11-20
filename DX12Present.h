// Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.  Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#ifndef _DX12_PRESENT_H_
#define _DX12_PRESENT_H_

#include <d3d12.h>
#include <dxgi1_4.h>

class DX12Present
{
public:
    DX12Present();
    ~DX12Present();
    bool Init(struct DX12SharedData* pSharedData);
    void Cleanup();
    bool Render();
    //bool VerifyResult();
    //bool CaptureFrame();
    //void WaitForCompletion();
    //bool WriteBMP(LPCSTR lpszFilename, LPCBYTE pPixels, UINT width, UINT rowPitch, UINT height);

private:
    HDC                                 m_hDC;
    IDXGIFactory2*                      m_pFactory;
    IDXGIAdapter1*                      m_pAdapter; 
    ID3D12Device*                       m_pDevice;
    IDXGISwapChain3*                    m_pSwapChain;
    ID3D12Resource*                     m_pRenderTargets[MAX_SHARED_BUFFERS];
    ID3D12CommandAllocator*             m_pCommandAllocator;
    ID3D12CommandQueue*                 m_pCommandQueue;
    ID3D12GraphicsCommandList*          m_pCommandList[MAX_SHARED_BUFFERS];

    //ID3D12Resource*                     m_pReadback;
    //D3D12_PLACED_SUBRESOURCE_FOOTPRINT* m_pReadbackTexLayout;
    //ID3D12Fence*                        m_pReadbackFence;
    //ID3D12GraphicsCommandList*          m_pReadbackCommandList[MAX_SHARED_BUFFERS];
    //HANDLE                              m_readbackFenceEvent;
    //UINT64                              m_readbackFenceValue;

    D3D12_VIEWPORT                      m_viewport;
    D3D12_RECT                          m_scissorRect;

    DX12SharedData*                     m_pSharedData;
    ID3D12Resource*                     m_pSharedMem[MAX_SHARED_BUFFERS];
    ID3D12Fence*                        m_pSharedFence[MAX_SHARED_BUFFERS];
    HANDLE                              m_sharedMemHandle[MAX_SHARED_BUFFERS];
    HANDLE                              m_sharedFenceHandle[MAX_SHARED_BUFFERS];
    UINT64                              m_sharedFenceValue[MAX_SHARED_BUFFERS];
    UINT                                m_frameIndex;
    UINT                                m_numFrames;
    bool                                m_initialized;
    ID3D12Debug* debugController = nullptr;
    ID3D12Debug1* debugController1 = nullptr;
};

#endif // _DX12_PRESENT_H_
