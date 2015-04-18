#include "pch.h"
#include "mp2tsource.h"
#include "mp2tstreamhandler.h"
#include <wrl\module.h>

using namespace ABI::mp2t;
ActivatableClass(Mp2tByteStreamHandler);
HRESULT CreateSourceReaderAsync(IMFByteStream *stream,
  IMFSourceReaderCallback *pCallback,
  IMFSourceReader **ppReader) {
  ComPtr<IMFAttributes> attrs;

  auto hr = MFCreateAttributes(attrs.ReleaseAndGetAddressOf(), 1);

  if (ok(hr))
    hr = attrs->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, pCallback);
  if (ok(hr))
    hr = MFCreateSourceReaderFromByteStream(stream, attrs.Get(), ppReader);
  return hr;
}
// Starts creating the media source.
HRESULT ABI::mp2t::Mp2tByteStreamHandler::BeginCreateObject(
  /* [in] */ IMFByteStream *stream,
  /* [in] */ LPCWSTR pwszURL,
  /* [in] */ DWORD dwFlags,
  /* [in] */ IPropertyStore *pProps,
  /* [out] */ IUnknown **ppIUnknownCancelCookie,  // Can be nullptr
  /* [in] */ IMFAsyncCallback *pCallback,
  /* [in] */ IUnknown *punkState) {       // Can be nullptr
  if (stream == nullptr || pCallback == nullptr)
    return E_POINTER;
  if ((dwFlags & MF_RESOLUTION_MEDIASOURCE) == 0)
    return E_INVALIDARG;

  ComPtr<IMFAsyncResult> outr;
  auto source = Make<Mp2tSource>();
  auto hr = MFCreateAsyncResult(Cast<IUnknown>(source).Get(), pCallback, punkState, &outr);
  if (failed(hr))
    return hr;

  hr = source->BeginOpen(stream, CreateAsyncCallback([outr, source](IMFAsyncResult* result) ->HRESULT {
    ComPtr<IMFSourceReader> reader;
    auto hr = source->EndOpen(result, &reader);
    reader;// ignore arg reader
    outr->SetStatus(hr);
    hr = MFInvokeCallback(outr.Get());
    //hr = MFPutWorkItemEx2(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, 0, outr.Get());
    return hr;
  }).Get(), nullptr);

  return hr;
}

ComPtr<IMFMediaSource> SourceFromResult(IMFAsyncResult*result) {
  ComPtr<IUnknown> ui;
  ComPtr<IMFMediaSource> ret;
  auto hr = result->GetObject(&ui);
  if (ok(hr))
    hr = ui.As(&ret);
  hr;// ignore return result
  return ret;
}
// Completes the BeginCreateObject operation.
HRESULT ABI::mp2t::Mp2tByteStreamHandler::EndCreateObject(
  /* [in] */ IMFAsyncResult *pResult,
  /* [out] */ MF_OBJECT_TYPE *objtype,
  /* [out] */ IUnknown **obj) {
  if (pResult == nullptr || objtype == nullptr || obj == nullptr)
    return E_POINTER;
  *obj = SourceFromResult(pResult).Detach();
  *objtype = MF_OBJECT_MEDIASOURCE;
  auto hr = pResult->GetStatus();
  return hr;
}

HRESULT ABI::mp2t::Mp2tByteStreamHandler::CancelObjectCreation(IUnknown *pIUnknownCancelCookie) {
  return E_NOTIMPL;
}

HRESULT ABI::mp2t::Mp2tByteStreamHandler::GetMaxNumberOfBytesRequiredForResolution(QWORD* pqwBytes) {
  return E_NOTIMPL;
}

ABI::mp2t::Mp2tByteStreamHandler::Mp2tByteStreamHandler() {}
ABI::mp2t::Mp2tByteStreamHandler::~Mp2tByteStreamHandler() {}

// IMediaExtension methods
// Sets the configuration of the media byte stream handler
HRESULT  ABI::mp2t::Mp2tByteStreamHandler::SetProperties(ABI::Windows::Foundation::Collections::IPropertySet *pConfiguration) {
  return S_OK;
}