/*------------------------------------.------------------------------.
| Filename: SmodeErrorAndAssert.cpp   | Error Assert                 |
| Author  : Alexandre Buge            |                              |
| Started : 25/11/2020 15:13          |                              |
` ----------------------------------- . --------------------------- */

#include "SmodeErrorAndAssert.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef GetCurrentTime // please unreal

# include <algorithm> // for remove

using namespace smode;

unsigned long smode::getLastSystemError()
{
  return GetLastError();
}

std::wstring smode::getSystemErrorMessage(unsigned long systemError)
{
  WCHAR buffer[1024] = { 0, };

  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, systemError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    buffer, (sizeof(buffer) / sizeof(*buffer)), NULL);

  std::wstring res(buffer); // remove line jump
  res.erase(std::remove(res.begin(), res.end(), L'\r'), res.end());
  res.erase(std::remove(res.begin(), res.end(), L'\n'), res.end());
  return res;
}

void smode::suspendCurrentThreadDuring(unsigned long milliseconds)
{
  assert(milliseconds); // starting to Windows Server 2003, current thread can deadlock if no more thread available.
  Sleep(milliseconds);
}

void smode::breakIfDebuggerPresent()
{
  if (IsDebuggerPresent() != FALSE)
    __debugbreak(); // Assertion break, see your callstack
}
