#pragma once

namespace ABI {
namespace mp2t {
class Mp2tByteStreamHandler : public RuntimeClass<RuntimeClassFlags< WinRtClassicComMix >
  , ABI::Windows::Media::IMediaExtension
  , IMFByteStreamHandler > {
  InspectableClass(L"mp2t.Mp2tByteStreamHandler", BaseTrust)

public:
  Mp2tByteStreamHandler();
  virtual ~Mp2tByteStreamHandler();

public:  // IMediaExtension
  STDMETHODIMP SetProperties(ABI::Windows::Foundation::Collections::IPropertySet *pConfiguration);

public:  // IMFAsyncCallback
  STDMETHODIMP GetParameters(DWORD *, DWORD *) { return E_NOTIMPL; } // todo: this is optional

  // IMFByteStreamHandler
  STDMETHODIMP BeginCreateObject(
    /* [in] */ IMFByteStream *pByteStream,
    /* [in] */ LPCWSTR pwszURL,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IPropertyStore *pProps,
    /* [out] */ IUnknown **ppIUnknownCancelCookie,
    /* [in] */ IMFAsyncCallback *pCallback,
    /* [in] */ IUnknown *punkState);

  STDMETHODIMP EndCreateObject(
    /* [in] */ IMFAsyncResult *pResult,
    /* [out] */ MF_OBJECT_TYPE *pObjectType,
    /* [out] */ IUnknown **ppObject);

  STDMETHODIMP CancelObjectCreation(IUnknown *pIUnknownCancelCookie);
  STDMETHODIMP GetMaxNumberOfBytesRequiredForResolution(QWORD *pqwBytes);
};
}
}
