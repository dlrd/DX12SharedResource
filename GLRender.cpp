/* -------------------------------------- . ---------------------------------- .
| Filename : GLRender.cpp                 |                                    |
| Author   : Alexandre Buge               |                                    |
| Started  : 14/10/2021 14:36             |                                    |
` --------------------------------------- . --------------------------------- */
#include "GLRender.h" 

#include <cmath>// for modf
#include "SmodeErrorAndAssert.h"
#include <type_traits> // for result_of
#include "GLExtensions.h"
#include "WGLExtensions.h"
#include <iostream>

/* ---------------------- Micro GL Wrangler ---------------- */

static bool hasGLErrorOccurred(GLenum expectedError, bool silentAssert = false )
{
  GLenum e;
  do
  {
    e = glGetError();
    switch (e)
    {
    case GL_INVALID_ENUM: assert(silentAssert); break;  // An unacceptable value is specified for an enumerated argument.
    case GL_INVALID_VALUE: assert(silentAssert); break; // A numeric argument is out of range.
    case GL_INVALID_OPERATION: assert(silentAssert); break; // The specified operation is not allowed in the current state.
    case GL_OUT_OF_MEMORY: assert(silentAssert); break; // Out of memory. undefined behavior
    case GL_INVALID_FRAMEBUFFER_OPERATION: assert(silentAssert); break; // Frame buffer error
    case GL_NO_ERROR: break; // Succeed
    default: assert(silentAssert); break; // Unknown error
    }
    if (e == expectedError)
      return true;
  } while (e != GL_NO_ERROR);
  return false;
}

static bool checkGLErrors()
{
#ifdef _DEBUG
  return hasGLErrorOccurred(GL_NO_ERROR);
#else // !_DEBUG
  return true;
#endif // !_DEBUG
}

template<typename FunctionType, size_t functionNameHash>
FunctionType __glLoadFunctionPointerIfNeeded(const char* functionName) // do not support GL context thread migration
{
  thread_local static FunctionType globalFunction = nullptr; // one per function type / name
  //std::cout << "functionName: " << functionName << "\t\tfunctionNameHash: " << std::hex << functionNameHash << std::dec << std::endl;
  if (!globalFunction)
    globalFunction = reinterpret_cast<FunctionType>(::wglGetProcAddress(functionName));
  assert(globalFunction);
  return globalFunction;
}

template<typename FunctionType, size_t functionNameHash, typename... Args>
void __glCall(const char* functionName, Args... args)
{
  __glLoadFunctionPointerIfNeeded<FunctionType, functionNameHash>(functionName)(args...);
  checkGLErrors();
}

template<typename FunctionType, size_t functionNameHash, typename... Args>
auto __glNonVoidCall(const char* functionName, Args... args) -> typename std::result_of<FunctionType(Args...)>::type
{
  auto res = __glLoadFunctionPointerIfNeeded<FunctionType, functionNameHash>(functionName)(args...);
  checkGLErrors();
  return res;
}
constexpr size_t __fnV1aStringHash(const char* const str, const size_t value = 0xcbf29ce484222325) noexcept // https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
  {return (str[0] == '\0') ? value : __fnV1aStringHash(&str[1], (value ^ size_t(str[0])) * 0x100000001b3);}

#define GL_CALL(X, ...) __glCall<PFN##X, __fnV1aStringHash(#X)>(#X, __VA_ARGS__);
#define GL_NON_VOID_CALL(X, ...) __glNonVoidCall<PFN##X, __fnV1aStringHash(#X)>(#X, __VA_ARGS__);

/* ---------------------------------------- */

static HGLRC createAndActivateGLContext(HDC hDC)
{
  // Init GL windows device context PixelFormat
  PIXELFORMATDESCRIPTOR pfd = { 0, };
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.iLayerType = PFD_MAIN_PLANE;
  pfd.cColorBits = 24;
  pfd.cRedBits = pfd.cGreenBits = pfd.cBlueBits = pfd.cAlphaBits = 8;
  pfd.cDepthBits = 16;
  const int pixelFormat = ChoosePixelFormat(hDC, &pfd);
  assert(pixelFormat);
  BOOL res = SetPixelFormat(hDC, pixelFormat, &pfd);
  assert(res);
  // Create and Bind GL Context
  HGLRC hRC = wglCreateContext(hDC);
  assert(hRC);
  res = wglMakeCurrent(hDC, hRC);
  assert(res);
  // Create and Bind Debug context
  static const int attributes[] =
  {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 6,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#ifdef _DEBUG
    | WGL_CONTEXT_DEBUG_BIT_ARB
#endif // _DEBUG
    , 0
  };
  HGLRC arbContext = GL_NON_VOID_CALL(wglCreateContextAttribsARB, hDC, nullptr, attributes);
  assert(arbContext);
  // delete old context
  res = wglMakeCurrent(nullptr, nullptr);
  assert(res);
  wglDeleteContext(hRC);
  hRC = arbContext;
  res = wglMakeCurrent(hDC, hRC); // and make new current
  assert(res);

#ifdef _DEBUG
  struct StaticOwner
  {
    static void GLAPIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
    {
      const char* debugSource;
      switch (source)
      {
      case GL_DEBUG_SOURCE_API_ARB: debugSource = "OpenGL"; break;
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: debugSource = "Windows"; break;
      case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: debugSource = "Shader Compiler"; break;
      case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: debugSource = "Third Party"; break;
      case GL_DEBUG_SOURCE_APPLICATION_ARB: debugSource = "Application"; break;
      case GL_DEBUG_SOURCE_OTHER_ARB:
      default:
        debugSource = "Other";
      }
      const char* debugType;
      switch (type)
      {
      case GL_DEBUG_TYPE_ERROR_ARB: debugType = "Error"; break;
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: debugType = "Deprecated behavior"; break;
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: debugType = "Undefined behavior"; break;
      case GL_DEBUG_TYPE_PORTABILITY_ARB: debugType = "Portability"; break;
      case GL_DEBUG_TYPE_PERFORMANCE_ARB: debugType = "Performance"; break;
      case GL_DEBUG_TYPE_OTHER_ARB:
      default:
        debugType = "Other"; break;
      }
      const char* debugSeverity;
      switch (severity)
      {
      case GL_DEBUG_SEVERITY_HIGH_ARB: debugSeverity = "High"; break;
      case GL_DEBUG_SEVERITY_MEDIUM_ARB: debugSeverity = "Medium"; break;
      case GL_DEBUG_SEVERITY_LOW_ARB: debugSeverity = "Low"; break;
      default:
        debugSeverity = "Other"; break;
      }

      std::cerr << debugSource << " " << debugType << " " << debugSeverity << " " << message;
      if (type == GL_DEBUG_TYPE_ERROR_ARB || type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB || type == GL_DEBUG_TYPE_PORTABILITY_ARB)
        assert_false; // on assert here, see "message" variable content
    }
  };

  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
  GL_CALL(glDebugMessageControlARB, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);  // activate all debug output
  GL_CALL(glDebugMessageCallbackARB, StaticOwner::debugCallback, nullptr);
#endif _DEBUG
  return hRC;
}

static void deactivateAndDeleteGLContext(HGLRC hRC)
{
  // Disable Debugging
#ifdef _DEBUG
  glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
  GL_CALL(glDebugMessageCallbackARB, nullptr, nullptr);
#endif // _DEBUG
  // Unbind and cleanup GL Context
  BOOL res = wglMakeCurrent(nullptr, nullptr);
  assert(res);
  res = wglDeleteContext(hRC);
  assert(res);
}

static bool checkFrameBufferStatus(GLenum target)
{
#ifdef _DEBUG
  GLenum status = GL_NON_VOID_CALL(glCheckFramebufferStatus, target);
  switch (status)
  {
    case GL_FRAMEBUFFER_COMPLETE: return true;  // OK
    case GL_FRAMEBUFFER_UNSUPPORTED: assert_false; return false; // Unsupported format.
    case GL_FRAMEBUFFER_UNDEFINED: assert_false; return false; // target is the default framebuffer, but the default framebuffer does not exist.
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: assert_false; return false; // Framebuffer incomplete, duplicate attachment.
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: assert_false; return false; // framebuffer does not have at least one image attached to it.
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: assert_false; return false; // Framebuffer incomplete, missing draw buffer.
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: assert_false; return false; // Framebuffer incomplete, missing read buffer.
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: assert_false; return false; // Multisample quality is not the same for all attached renderbuffers.
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: assert_false; return false; // Number of layers differ between attachments.
    default: checkGLErrors(); assert_false; return false; // unkown framebuffer error.
  }
#else // _DEBUG
  return true;
#endif // _DEBUG
}

bool GLRender::Init(DX12SharedData* pSharedData)
{
  // create gl context
  this->pSharedData = pSharedData;
  hDC = GetDC(pSharedData->hWnd);
  assert(hDC);
  hRC = createAndActivateGLContext(hDC);
  assert(hRC);
  const GLubyte* vendor = glGetString(GL_VENDOR);
  std::cout << "Vendor: " << vendor << "\n";
  // share objects
  for (UINT i = 0; i < pSharedData->numSharedBuffers; ++i)
  {
    GL_CALL(glGenSemaphoresEXT, 1, &buffers[i].semaphore);
    GL_CALL(glImportSemaphoreWin32HandleEXT, buffers[i].semaphore, GL_HANDLE_TYPE_D3D12_FENCE_EXT, pSharedData->sharedFenceHandle[i]);
    GLboolean res = GL_NON_VOID_CALL(glIsSemaphoreEXT, buffers[i].semaphore);
    assert(res);
    GL_CALL(glCreateMemoryObjectsEXT, 1, &buffers[i].memoryObject);
    //const GLuint64 importSize = GLuint64(pSharedData->width) * GLuint64(pSharedData->height) * 4/* GL_RGBA8 */; /* fixme find dx12 equivalent to vkGetBufferMemoryRequirements? */
    GL_CALL(glImportMemoryWin32HandleEXT, buffers[i].memoryObject, 0, GL_HANDLE_TYPE_D3D12_RESOURCE_EXT, pSharedData->sharedMemHandle[i]);
    GL_CALL(glCreateTextures, GL_TEXTURE_2D, 1, &buffers[i].textureId);
    GL_CALL(glTextureStorageMem2DEXT, buffers[i].textureId, 1, GL_RGBA8, pSharedData->width, pSharedData->height, buffers[i].memoryObject, 0 /* fixme maybe not 0 offset on DX12 texture? */);
    GL_CALL(glTextureParameteri, buffers[i].textureId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GL_CALL(glTextureParameteri, buffers[i].textureId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GL_CALL(glTextureParameteri, buffers[i].textureId, GL_TEXTURE_WRAP_S, GL_REPEAT);
    GL_CALL(glTextureParameteri, buffers[i].textureId, GL_TEXTURE_WRAP_T, GL_REPEAT);
    buffers[i].rendered = false;
  }
  // create a framebuffer on the shared texture
  GL_CALL(glGenFramebuffers, 1, &frameBuffer);
  initialized = true;
  return true;
}

void GLRender::Cleanup()
{
  initialized = false;
  // cleanup framebuffer
  GL_CALL(glBindFramebuffer, GL_DRAW_FRAMEBUFFER, 0); // just in case our framebuffer is bound
  GL_CALL(glDeleteFramebuffers, 1, &frameBuffer);
  frameBuffer = 0;
  // cleanup sharing
  for (UINT i = 0; i < pSharedData->numSharedBuffers; ++i)
  {
    glDeleteTextures(1, &buffers[i].textureId);
    checkGLErrors();
    GL_CALL(glDeleteMemoryObjectsEXT, 1, &buffers[i].memoryObject);
    GL_CALL(glDeleteSemaphoresEXT, 1, &buffers[i].semaphore);
  } 
  // delete gl context
  deactivateAndDeleteGLContext(hRC);
  hRC = nullptr;
  // Release device Context
  ReleaseDC(pSharedData->hWnd, hDC);
  hDC = nullptr;
}

static void paintIntoCurrentDrawFramebuffer()
{
  static int frameCount = 0;
  ++frameCount;
  std::cout << "frameCount: " << frameCount << std::endl;

  float ipart = 0.f;
  float clamped = std::modf(float(frameCount) / 100.f, &ipart);
  glClearColor(clamped, clamped, clamped, 1.0f);
  checkGLErrors();
  glClear(GL_COLOR_BUFFER_BIT);
  checkGLErrors();
}

void GLRender::Render()
{
  const uint32_t currentBuffer = pSharedData->currentBufferIndex;
  
  GLenum srcLayout = GL_LAYOUT_COLOR_ATTACHMENT_EXT;
  GL_CALL(glSemaphoreParameterui64vEXT, buffers[currentBuffer].semaphore, GL_D3D12_FENCE_VALUE_EXT, &buffers[currentBuffer].semaphoreFenceValue);
  GL_CALL(glWaitSemaphoreEXT, buffers[currentBuffer].semaphore, 0, nullptr, 1, &buffers[currentBuffer].textureId, &srcLayout);
  
  // fill texture thanks to framebuffer renderer technics
  GL_CALL(glBindFramebuffer, GL_DRAW_FRAMEBUFFER, frameBuffer);
  GL_CALL(glFramebufferTexture2D, GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffers[currentBuffer].textureId, 0);
  
  checkFrameBufferStatus(GL_DRAW_FRAMEBUFFER);
  glViewport(0, 0, pSharedData->width, pSharedData->height);
  checkGLErrors();

  paintIntoCurrentDrawFramebuffer();
  buffers[currentBuffer].semaphoreFenceValue++;
  buffers[currentBuffer].semaphoreFenceValue++;
  std::cout << buffers[currentBuffer].semaphoreFenceValue << std::endl; 
  GL_CALL(glBindFramebuffer, GL_DRAW_FRAMEBUFFER, 0);
  GL_CALL(glSemaphoreParameterui64vEXT, buffers[currentBuffer].semaphore, GL_D3D12_FENCE_VALUE_EXT, &buffers[currentBuffer].semaphoreFenceValue);
  GL_CALL(glSignalSemaphoreEXT, buffers[currentBuffer].semaphore, 0, nullptr, 1, &buffers[currentBuffer].textureId, &srcLayout);

  buffers[currentBuffer].rendered = true;
}

bool GLRender::Initialized()
  {return initialized;}

AbstractRender* newGLRender()
  {return new GLRender();}
