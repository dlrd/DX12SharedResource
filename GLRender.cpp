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

template<typename FunctionType>
FunctionType __loadGLFunctionPointerIfNeeded(const char* functionName)
{
  static FunctionType globalFunction = nullptr; // one per function type 
  if (!globalFunction) 
    globalFunction = reinterpret_cast<FunctionType>(::wglGetProcAddress(functionName));
  assert(globalFunction);
  return globalFunction;
}

template<typename FunctionType, typename... Args>
void __glCall(const char* functionName, Args... args)
{
  __loadGLFunctionPointerIfNeeded<FunctionType>(functionName)(args...);
  checkGLErrors();
}

template<typename FunctionType, typename... Args>
auto __glNonVoidCall(const char* functionName, Args... args) -> typename std::result_of<FunctionType(Args...)>::type
{
  auto res = __loadGLFunctionPointerIfNeeded<FunctionType>(functionName)(args...);
  checkGLErrors();
  return res;
}

#define GL_CALL(X, ...) __glCall<PFN##X>(#X, __VA_ARGS__);
#define GL_NON_VOID_CALL(X, ...) __glNonVoidCall<PFN##X>(#X, __VA_ARGS__);

/* ---------------- ----------------------- */

static bool hasGLErrorOccurred(GLenum waitedError, bool silentAssert = false )
{
  GLenum e;
  do
  {
    e = glGetError();
    switch (e)
    {
      // An unacceptable value is specified for an enumerated argument. ignored.
    case GL_INVALID_ENUM: if (silentAssert) {}
                        else { assert_false; }; break;
      // A numeric argument is out of range. ignored.
    case GL_INVALID_VALUE: if (silentAssert) {}
                         else { assert_false; } break;
      // The specified operation is not allowed in the current state. ignored.
    case GL_INVALID_OPERATION: if (silentAssert) {}
                             else { assert_false; } break;
      // Out of memory. undefined behavior
    case GL_OUT_OF_MEMORY: if (silentAssert) {}
                         else { assert_false; } break;
      // Frame buffer error*
    case GL_INVALID_FRAMEBUFFER_OPERATION: if (silentAssert) {}
                                         else { assert_false; } break;
      // Succeed
    case GL_NO_ERROR: break;
      // Unknown error
    default: if (silentAssert) {}
           else { silentAssert; } break;
    }
    if (e == waitedError)
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

static HGLRC createAndActivateGLContext(HDC hDC)
{
  // Init GL windows device context PixelFormat
  PIXELFORMATDESCRIPTOR pfd = { 0, };
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
  pfd.dwFlags |= PFD_SWAP_EXCHANGE;
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
    | WGL_CONTEXT_DEBUG_BIT_ARB,
#endif // _DEBUG
    0
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

bool GLRender::Init(DX12SharedData* pSharedData)
{
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

    GL_CALL(glCreateBuffers, 1, &buffers[i].bufferId);
    GL_CALL(glCreateMemoryObjectsEXT, 1, &buffers[i].memoryObject);

    const GLuint64 importSize = GLuint64(pSharedData->width) * GLuint64(pSharedData->height) * 4/* RGBAU8 */; /* fixme find dx12 equivalent to vkGetBufferMemoryRequirements? */
    GL_CALL(glImportMemoryWin32HandleEXT, buffers[i].memoryObject, importSize, GL_HANDLE_TYPE_D3D12_RESOURCE_EXT, pSharedData->sharedMemHandle[i]);
  }

  initialized = true;
  return true;
}

void GLRender::Cleanup()
{
  initialized = false;

  for (UINT i = 0; i < pSharedData->numSharedBuffers; ++i)
  {
    GL_CALL(glDeleteBuffers, 1, &buffers[i].bufferId);
    GL_CALL(glDeleteMemoryObjectsEXT, 1, &buffers[i].memoryObject);
    GL_CALL(glDeleteSemaphoresEXT, 1, &buffers[i].semaphore);
  }

  deactivateAndDeleteGLContext(hRC);
  hRC = nullptr;
  // Release device Context
  ReleaseDC(pSharedData->hWnd, hDC);
  hDC = nullptr;
}

void GLRender::Render()
{
  static int a = 0;
  ++a;
  float ipart = 0.f;
  float clamped = std::modf(float(a) / 100.f, &ipart);

  glClearColor(clamped, clamped, clamped, 1.0f);
}

bool GLRender::Initialized()
  {return initialized;}

AbstractRender* newAbstractRender()
  {return new GLRender();}
