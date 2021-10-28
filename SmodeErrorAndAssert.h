/*------------------------------------.------------------------------.
| Filename: SmodeErrorAndAssert.h     | Error and Assert             |
| Author  : Alexandre Buge            |                              | 
| Started : 25/11/2020 15:13          |                              |
` ----------------------------------- . --------------------------- */

#ifndef PLUGINS_VIDEO_PIPE_ASSERT_H_
# define PLUGINS_VIDEO_PIPE_ASSERT_H_

# include <string> // wstring

# ifdef _MSC_VER
#  define SMODE_FORCE_CALLER_TO_ADD_SEMICOLON(x) \
   __pragma(warning(push)) \
   __pragma(warning(disable:4127)) \
   do { x } while (false) \
   __pragma(warning(pop))
# else // _MSC_VER
#  define SMODE_FORCE_USER_TO_PUT_SEMICOLON(x) do { x } while (false) 
# endif // _MSC_VER 

# ifdef _DEBUG 
#  define assert_false smode::breakIfDebuggerPresent()
#  define assert(x) if (!(x)) assert_false
# else // _DEBUG
#  define assert_false SMODE_FORCE_CALLER_TO_ADD_SEMICOLON(;)
#  define assert(x) SMODE_FORCE_CALLER_TO_ADD_SEMICOLON(;)
# endif // _DEBUG

namespace smode
{

unsigned long getLastSystemError();

std::wstring getSystemErrorMessage(unsigned long systemError);

void breakIfDebuggerPresent();

void suspendCurrentThreadDuring(unsigned long milliseconds);

}; /* namespace smode */

#endif // !PLUGINS_VIDEO_PIPE_ASSERT_H_
