#pragma once
#include "mp2t.h"

/*
Within each work queue, the callbacks are serialized. If you call MFPutWorkItem twice with the same work queue, the second callback is not invoked until the first callback has returned.
*/

inline HRESULT Async(IMFAsyncCallback *call, IUnknown*stat = nullptr) {
  return MFPutWorkItem2(MFASYNC_CALLBACK_QUEUE_STANDARD, 0, call, stat);
}
inline HRESULT Async(InvokeFun work, IUnknown*stat = nullptr) {
  auto invoke = CreateAsyncCallback(work);
  return Async(invoke.Get(), stat);
}
