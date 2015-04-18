#pragma once
#include "mp2t.h"

/*
Within each work queue, the callbacks are serialized. If you call MFPutWorkItem twice with the same work queue, the second callback is not invoked until the first callback has returned.
*/
//MFASYNC_CALLBACK_QUEUE_MULTITHREADED
inline HRESULT Async(IMFAsyncCallback *call, DWORD qid = MFASYNC_CALLBACK_QUEUE_STANDARD, IUnknown*stat = nullptr) {
  return MFPutWorkItem2(MFASYNC_CALLBACK_QUEUE_STANDARD, 0, call, stat);
}
inline HRESULT Async(InvokeFun work, IUnknown*stat = nullptr) {
  auto invoke = CreateAsyncCallback(work);
  return Async(invoke.Get(), MFASYNC_CALLBACK_QUEUE_STANDARD, stat);
}
inline HRESULT Async2(InvokeFun work, IUnknown*stat = nullptr) {
  auto invoke = CreateAsyncCallback(work);
  return Async(invoke.Get(), MFASYNC_CALLBACK_QUEUE_MULTITHREADED, stat);
}
