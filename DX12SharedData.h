// Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.  Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#ifndef _DX12_SHARED_DATA_H_
#define _DX12_SHARED_DATA_H_

#include <windows.h>


struct DX12SharedData {
  LUID AdapterLuid;
  HWND hWnd;
  UINT width;
  UINT height;
  UINT numSharedBuffers;
  HANDLE sharedMemHandle[MAX_SHARED_BUFFERS];
  HANDLE sharedFenceHandle[MAX_SHARED_BUFFERS];
  HANDLE startEvent;
  HANDLE doneEvent;
  UINT currentBufferIndex;
  bool forceDedicatedMemory;
  bool terminate;
  //bool verify;
  bool vsync;
  bool terminated;
  //UINT captureFrame;
  //LPCSTR captureFile;
};

class AbstractRender
{
public:
  virtual ~AbstractRender() {}

  virtual bool Init(DX12SharedData* pSharedData) = 0;
  virtual void Cleanup() = 0;
  virtual void Render() = 0;
  virtual bool Initialized() = 0;
};

extern AbstractRender* newAbstractRender();

#endif // _DX12_SHARED_DATA_H_
