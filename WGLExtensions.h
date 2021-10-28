/* -------------------------------------- . ---------------------------------- .
| Filename : WGLExtensions.h              | function typedef name must be      |
| Author   : Alexandre Buge               | the function name prefixed by      |
| Started  : 21/10/2021 19:38             | PFN (adapted from wGlew header)    |
` --------------------------------------- . --------------------------------- */

#ifndef _WGL_EXTENSIONS_H_
#define _WGL_EXTENSIONS_H_

/* ------------------------- WGL_ARB_create_context ------------------------ */

#define WGL_CONTEXT_DEBUG_BIT_ARB 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define ERROR_INVALID_VERSION_ARB 0x2095
#define ERROR_INVALID_PROFILE_ARB 0x2096

typedef HGLRC(WINAPI* PFNwglCreateContextAttribsARB) (HDC hDC, HGLRC hShareContext, const int* attribList);


/* --------------------- WGL_ARB_create_context_profile -------------------- */

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

#endif // _WGL_EXTENSIONS_H_
