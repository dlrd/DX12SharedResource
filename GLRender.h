/* -------------------------------------- . ---------------------------------- .
| Filename : GLRender.h                   |                                    |
| Author   : Alexandre Buge               |                                    |
| Started  : 14/10/2021 14:36             |                                    |
` --------------------------------------- . --------------------------------- */

#ifndef _GL_RENDER_H_
#define _GL_RENDER_H_

// platform defines for GL.h
#define WINGDIAPI __declspec(dllimport)
#define APIENTRY __stdcall

#include <gl/GL.h>
#include "DX12SharedData.h" // for AbstractRender

class GLRender : public AbstractRender
{
 public:
   bool Init(DX12SharedData* pSharedData) override;
   void Cleanup() override;
   void Render() override;
   bool Initialized() override;

private:

  struct
  {
    GLuint semaphore;
    GLuint memoryObject;
    GLuint bufferId;
  } buffers[MAX_SHARED_BUFFERS] = { 0, };

  DX12SharedData* pSharedData = nullptr;
  HGLRC hRC = nullptr;
  HDC hDC = nullptr;
  bool initialized = false;
};

#endif // _GL_RENDER_H_
