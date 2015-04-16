#include "pch.h"
#include "mp2tsource.h"
#include "mp2tstreamhandler.h"
#include <wrl\module.h>

using namespace ABI::mp2t;
ActivatableClass(Mp2tByteStreamHandler);

//-------------------------------------------------------------------
// CMPEG1ByteStreamHandler  class
//-------------------------------------------------------------------
//-------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------

ABI::mp2t::Mp2tByteStreamHandler::Mp2tByteStreamHandler()
{
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------

ABI::mp2t::Mp2tByteStreamHandler::~Mp2tByteStreamHandler()
{
}

//-------------------------------------------------------------------
// IMediaExtension methods
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// SetProperties
// Sets the configuration of the media byte stream handler
//-------------------------------------------------------------------
HRESULT  ABI::mp2t::Mp2tByteStreamHandler::SetProperties(ABI::Windows::Foundation::Collections::IPropertySet *pConfiguration)
{
  return S_OK;
}

//-------------------------------------------------------------------
// IMFByteStreamHandler methods
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// BeginCreateObject
// Starts creating the media source.
//-------------------------------------------------------------------

HRESULT ABI::mp2t::Mp2tByteStreamHandler::BeginCreateObject(
  /* [in] */ IMFByteStream *pByteStream,
  /* [in] */ LPCWSTR pwszURL,
  /* [in] */ DWORD dwFlags,
  /* [in] */ IPropertyStore *pProps,
  /* [out] */ IUnknown **ppIUnknownCancelCookie,  // Can be nullptr
  /* [in] */ IMFAsyncCallback *pCallback,
  /* [in] */ IUnknown *punkState                  // Can be nullptr
  )
{
  HRESULT hr = S_OK;
  try
  {
    if (pByteStream == nullptr)
    {
      ThrowException(E_POINTER);
    }

    if (pCallback == nullptr)
    {
      ThrowException(E_POINTER);
    }

    if ((dwFlags & MF_RESOLUTION_MEDIASOURCE) == 0)
    {
      ThrowException(E_INVALIDARG);
    }

    ComPtr<IMFAsyncResult> spResult;
    ComPtr<Mp2tSource> spSource = Make<Mp2tSource>();//MakeMp2tSource();

    ComPtr<IUnknown> spSourceUnk;
    ThrowIfError(spSource.As(&spSourceUnk));
    ThrowIfError(MFCreateAsyncResult(spSourceUnk.Get(), pCallback, punkState, &spResult));

    // Start opening the source. This is an async operation.
    // When it completes, the source will invoke our callback
    // and then we will invoke the caller's callback.
    ComPtr<Mp2tByteStreamHandler> spThis = this;
    spSource->OpenAsync(pByteStream).then([this, spThis, spResult, spSource](concurrency::task<void>& openTask)
    {
      try
      {
        if (spResult == nullptr)
        {
          ThrowIfError(MF_E_UNEXPECTED);
        }

        openTask.get();
      }
      catch (Exception ^exc)
      {
        if (spResult != nullptr)
        {
          spResult->SetStatus(exc->HResult);
        }
      }

      if (spResult != nullptr)
      {
        MFInvokeCallback(spResult.Get());
      }
    });

    if (ppIUnknownCancelCookie)
    {
      *ppIUnknownCancelCookie = nullptr;
    }
  }
  catch (Exception ^exc)
  {
    hr = exc->HResult;
  }

  return hr;
}

//-------------------------------------------------------------------
// EndCreateObject
// Completes the BeginCreateObject operation.
//-------------------------------------------------------------------

HRESULT ABI::mp2t::Mp2tByteStreamHandler::EndCreateObject(
  /* [in] */ IMFAsyncResult *pResult,
  /* [out] */ MF_OBJECT_TYPE *pObjectType,
  /* [out] */ IUnknown **ppObject)
{
  if (pResult == nullptr || pObjectType == nullptr || ppObject == nullptr)
  {
    return E_POINTER;
  }

  HRESULT hr = S_OK;

  *pObjectType = MF_OBJECT_INVALID;
  *ppObject = nullptr;

  hr = pResult->GetStatus();

  if (SUCCEEDED(hr))
  {
    ComPtr<IUnknown> punkSource;
    hr = pResult->GetObject(&punkSource);
    if (SUCCEEDED(hr))
    {
      *pObjectType = MF_OBJECT_MEDIASOURCE;
      *ppObject = punkSource.Detach();
    }
  }

  return hr;
}

HRESULT ABI::mp2t::Mp2tByteStreamHandler::CancelObjectCreation(IUnknown *pIUnknownCancelCookie)
{
  return E_NOTIMPL;
}

HRESULT ABI::mp2t::Mp2tByteStreamHandler::GetMaxNumberOfBytesRequiredForResolution(QWORD* pqwBytes)
{
  return E_NOTIMPL;
}