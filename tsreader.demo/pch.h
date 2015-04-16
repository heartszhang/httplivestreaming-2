// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include <wrl.h>

#include <objbase.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

#include <stdio.h>
#include <tchar.h>

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfreadwrite.lib")
// TODO: reference additional headers your program requires here

inline bool ok(HRESULT hr) { return SUCCEEDED(hr); }
inline bool failed(HRESULT hr) { return FAILED(hr); }

template<typename I> using ComPtr = Microsoft::WRL::ComPtr<I>;