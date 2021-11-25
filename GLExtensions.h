/* -------------------------------------- . ---------------------------------- .
| Filename : GLExtensions.h               | function typedef name must be      |
| Author   : Alexandre Buge               | the function name prefixed by      |
| Started  : 21/10/2021 19:38             | PFN (adapted from Glew header)     |
` --------------------------------------- . --------------------------------- */

#ifndef _GL_EXTENSIONS_H_
#define _GL_EXTENSIONS_H_

#include <cstdint> // for int64_t && uint64_t

#define GLAPIENTRY APIENTRY
typedef uint64_t GLuint64;
typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;

/* ----------------------------- GL_VERSION_1_5 ---------------------------- */

typedef void (GLAPIENTRY* PFNglDeleteBuffers) (GLsizei n, const GLuint* buffers);
typedef GLboolean(GLAPIENTRY* PFNglIsBuffer) (GLuint buffer);

/* ----------------------------- GL_VERSION_3_2 ---------------------------- */

#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8

/* ----------------------- GL_ARB_framebuffer_object ----------------------- */

#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506

/* ----------------------- GL_ARB_direct_state_access ---------------------- */

typedef void (GLAPIENTRY* PFNglCreateBuffers) (GLsizei n, GLuint* buffers);
typedef void (GLAPIENTRY* PFNglCreateTextures) (GLenum target, GLsizei n, GLuint* textures);
typedef void (GLAPIENTRY* PFNglTextureParameteri) (GLuint texture, GLenum pname, GLint param);
typedef void (GLAPIENTRY* PFNglGenerateTextureMipmap) (GLuint texture);
typedef void (GLAPIENTRY* PFNglTextureSubImage2D) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);

/* ---------------------------- GL_EXT_semaphore --------------------------- */

typedef void (GLAPIENTRY* PFNglDeleteSemaphoresEXT) (GLsizei n, const GLuint* semaphores);
typedef void (GLAPIENTRY* PFNglGenSemaphoresEXT) (GLsizei n, GLuint* semaphores);
typedef void (GLAPIENTRY* PFNglGetSemaphoreParameterui64vEXT) (GLuint semaphore, GLenum pname, GLuint64* params);
typedef GLboolean(GLAPIENTRY* PFNglIsSemaphoreEXT) (GLuint semaphore);
typedef void (GLAPIENTRY* PFNglSemaphoreParameterui64vEXT) (GLuint semaphore, GLenum pname, const GLuint64* params);
typedef void (GLAPIENTRY* PFNglSignalSemaphoreEXT) (GLuint semaphore, GLuint numBufferBarriers, const GLuint* buffers, GLuint numTextureBarriers, const GLuint* textures, const GLenum* dstLayouts);
typedef void (GLAPIENTRY* PFNglWaitSemaphoreEXT) (GLuint semaphore, GLuint numBufferBarriers, const GLuint* buffers, GLuint numTextureBarriers, const GLuint* textures, const GLenum* srcLayouts);

/* ------------------------- GL_EXT_semaphore_win32 ------------------------ */

typedef void (GLAPIENTRY* PFNglImportSemaphoreWin32HandleEXT) (GLuint semaphore, GLenum handleType, void* handle);
typedef void (GLAPIENTRY* PFNglImportSemaphoreWin32NameEXT) (GLuint semaphore, GLenum handleType, const void* name);

/* -------------------------- GL_EXT_memory_object ------------------------- */

#define GL_UUID_SIZE_EXT 16
#define GL_TEXTURE_TILING_EXT 0x9580
#define GL_DEDICATED_MEMORY_OBJECT_EXT 0x9581
#define GL_NUM_TILING_TYPES_EXT 0x9582
#define GL_TILING_TYPES_EXT 0x9583
#define GL_OPTIMAL_TILING_EXT 0x9584
#define GL_LINEAR_TILING_EXT 0x9585
#define GL_LAYOUT_GENERAL_EXT 0x958D
#define GL_LAYOUT_COLOR_ATTACHMENT_EXT 0x958E
#define GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT 0x958F
#define GL_LAYOUT_DEPTH_STENCIL_READ_ONLY_EXT 0x9590
#define GL_LAYOUT_SHADER_READ_ONLY_EXT 0x9591
#define GL_LAYOUT_TRANSFER_SRC_EXT 0x9592
#define GL_LAYOUT_TRANSFER_DST_EXT 0x9593
#define GL_NUM_DEVICE_UUIDS_EXT 0x9596
#define GL_DEVICE_UUID_EXT 0x9597
#define GL_DRIVER_UUID_EXT 0x9598
#define GL_PROTECTED_MEMORY_OBJECT_EXT 0x959B

typedef void (GLAPIENTRY* PFNglBufferStorageMemEXT) (GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNglCreateMemoryObjectsEXT) (GLsizei n, GLuint* memoryObjects);
typedef void (GLAPIENTRY* PFNglDeleteMemoryObjectsEXT) (GLsizei n, const GLuint* memoryObjects);
typedef void (GLAPIENTRY* PFNglGetMemoryObjectParameterivEXT) (GLuint memoryObject, GLenum pname, GLint* params);
typedef void (GLAPIENTRY* PFNglGetUnsignedBytei_vEXT) (GLenum target, GLuint index, GLubyte* data);
typedef void (GLAPIENTRY* PFNglGetUnsignedBytevEXT) (GLenum pname, GLubyte* data);
typedef GLboolean(GLAPIENTRY* PFNglIsMemoryObjectEXT) (GLuint memoryObject);
typedef void (GLAPIENTRY* PFNglMemoryObjectParameterivEXT) (GLuint memoryObject, GLenum pname, const GLint* params);
typedef void (GLAPIENTRY* PFNglNamedBufferStorageMEXT) (GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNglTexStorageMem1DEXT) (GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNglTexStorageMem2DEXT) (GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNglTexStorageMem2DMultisampleEXT) (GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNglTexStorageMem3DEXT) (GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNglTexStorageMem3DMultisampleEXT) (GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNglTextureStorageMem1DEXT) (GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNglTextureStorageMem2DEXT) (GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNglTextureStorageMem2DMultiSampleEXT) (GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNflTextureStorageMem3DEXT) (GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset);
typedef void (GLAPIENTRY* PFNGLTextureStorageMem3DMultisampleEXT) (GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset);

/* ----------------------- GL_EXT_memory_object_win32 ---------------------- */

#define GL_LUID_SIZE_EXT 8
#define GL_HANDLE_TYPE_OPAQUE_WIN32_EXT 0x9587
#define GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT 0x9588
#define GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT 0x9589
#define GL_HANDLE_TYPE_D3D12_RESOURCE_EXT 0x958A
#define GL_HANDLE_TYPE_D3D11_IMAGE_EXT 0x958B
#define GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT 0x958C
#define GL_HANDLE_TYPE_D3D12_FENCE_EXT 0x9594
#define GL_D3D12_FENCE_VALUE_EXT 0x9595
#define GL_DEVICE_LUID_EXT 0x9599
#define GL_DEVICE_NODE_MASK_EXT 0x959A

typedef void (GLAPIENTRY* PFNglImportMemoryWin32HandleEXT) (GLuint memory, GLuint64 size, GLenum handleType, void* handle);
typedef void (GLAPIENTRY* PFNglImportMemoryWin32NameEXT) (GLuint memory, GLuint64 size, GLenum handleType, const void* name);

/* -------------------------- GL_ARB_debug_output -------------------------- */

#define GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB 0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB 0x8243
#define GL_DEBUG_CALLBACK_FUNCTION_ARB 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM_ARB 0x8245
#define GL_DEBUG_SOURCE_API_ARB 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER_ARB 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY_ARB 0x8249
#define GL_DEBUG_SOURCE_APPLICATION_ARB 0x824A
#define GL_DEBUG_SOURCE_OTHER_ARB 0x824B
#define GL_DEBUG_TYPE_ERROR_ARB 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB 0x824E
#define GL_DEBUG_TYPE_PORTABILITY_ARB 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE_ARB 0x8250
#define GL_DEBUG_TYPE_OTHER_ARB 0x8251
#define GL_MAX_DEBUG_MESSAGE_LENGTH_ARB 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES_ARB 0x9144
#define GL_DEBUG_LOGGED_MESSAGES_ARB 0x9145
#define GL_DEBUG_SEVERITY_HIGH_ARB 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_ARB 0x9147
#define GL_DEBUG_SEVERITY_LOW_ARB 0x9148

typedef void (GLAPIENTRY* GLDEBUGPROCARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

typedef void (GLAPIENTRY* PFNglDebugMessageCallbackARB) (GLDEBUGPROCARB callback, const void* userParam);
typedef void (GLAPIENTRY* PFNglDebugMessageControlARB) (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled);
typedef void (GLAPIENTRY* PFNglDebugMessageInsertARB) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* buf);
typedef GLuint(GLAPIENTRY* PFNglGetDebugMessageLogARB) (GLuint count, GLsizei bufSize, GLenum* sources, GLenum* types, GLuint* ids, GLenum* severities, GLsizei* lengths, GLchar* messageLog);

/* ----------------------- GL_ARB_framebuffer_object ----------------------- */

#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING 0x8210
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE 0x8211
#define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE 0x8212
#define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE 0x8213
#define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE 0x8214
#define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE 0x8215
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE 0x8216
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE 0x8217
#define GL_FRAMEBUFFER_DEFAULT 0x8218
#define GL_FRAMEBUFFER_UNDEFINED 0x8219
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_INDEX 0x8222
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#define GL_DEPTH_STENCIL 0x84F9
#define GL_UNSIGNED_INT_24_8 0x84FA
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_TEXTURE_STENCIL_SIZE 0x88F1
#define GL_UNSIGNED_NORMALIZED 0x8C17
#define GL_SRGB 0x8C40
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_RENDERBUFFER_BINDING 0x8CA7
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_RENDERBUFFER_SAMPLES 0x8CAB
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS 0x8CDF
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_COLOR_ATTACHMENT4 0x8CE4
#define GL_COLOR_ATTACHMENT5 0x8CE5
#define GL_COLOR_ATTACHMENT6 0x8CE6
#define GL_COLOR_ATTACHMENT7 0x8CE7
#define GL_COLOR_ATTACHMENT8 0x8CE8
#define GL_COLOR_ATTACHMENT9 0x8CE9
#define GL_COLOR_ATTACHMENT10 0x8CEA
#define GL_COLOR_ATTACHMENT11 0x8CEB
#define GL_COLOR_ATTACHMENT12 0x8CEC
#define GL_COLOR_ATTACHMENT13 0x8CED
#define GL_COLOR_ATTACHMENT14 0x8CEE
#define GL_COLOR_ATTACHMENT15 0x8CEF
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_RENDERBUFFER_WIDTH 0x8D42
#define GL_RENDERBUFFER_HEIGHT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#define GL_STENCIL_INDEX1 0x8D46
#define GL_STENCIL_INDEX4 0x8D47
#define GL_STENCIL_INDEX8 0x8D48
#define GL_STENCIL_INDEX16 0x8D49
#define GL_RENDERBUFFER_RED_SIZE 0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE 0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE 0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE 0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE 0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE 0x8D55
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#define GL_MAX_SAMPLES 0x8D57

typedef void (GLAPIENTRY* PFNglBindFramebuffer) (GLenum target, GLuint framebuffer);
typedef void (GLAPIENTRY* PFNglBindRenderbuffer) (GLenum target, GLuint renderbuffer);
typedef void (GLAPIENTRY* PFNglBlitFramebuffer) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef GLenum(GLAPIENTRY* PFNglCheckFramebufferStatus) (GLenum target);
typedef void (GLAPIENTRY* PFNglDeleteFramebuffers) (GLsizei n, const GLuint* framebuffers);
typedef void (GLAPIENTRY* PFNglDeleteRenderbuffers) (GLsizei n, const GLuint* renderbuffers);
typedef void (GLAPIENTRY* PFNglFramebufferRenderbuffer) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (GLAPIENTRY* PFNglFramebufferTexture1D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (GLAPIENTRY* PFNglFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (GLAPIENTRY* PFNglFramebufferTexture3D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer);
typedef void (GLAPIENTRY* PFNglFramebufferTextureLayer) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
typedef void (GLAPIENTRY* PFNglGenFramebuffers) (GLsizei n, GLuint* framebuffers);
typedef void (GLAPIENTRY* PFNglGenRenderbuffers) (GLsizei n, GLuint* renderbuffers);
typedef void (GLAPIENTRY* PFNglGenerateMipmap) (GLenum target);
typedef void (GLAPIENTRY* PFNglGetFramebufferAttachmentParameteriv) (GLenum target, GLenum attachment, GLenum pname, GLint* params);
typedef void (GLAPIENTRY* PFNglGetRenderbufferParameteriv) (GLenum target, GLenum pname, GLint* params);
typedef GLboolean(GLAPIENTRY* PFNglIsFramebuffer) (GLuint framebuffer);
typedef GLboolean(GLAPIENTRY* PFNglIsRenderbuffer) (GLuint renderbuffer);
typedef void (GLAPIENTRY* PFNglRenderbufferStorage) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GLAPIENTRY* PFNglRenderbufferStorageMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

#endif // _GL_EXTENSIONS_H_
