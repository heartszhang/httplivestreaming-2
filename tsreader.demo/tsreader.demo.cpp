// TsReaderDemo.cpp : Defines the entry point for the console application.
//

#include "pch.h"
#include <strmif.h> // AM_MEDIA_TYPE

HRESULT PrintMediaTypeDetail(BOOL zipped, const AM_MEDIA_TYPE*mt) {
  zipped; mt;
  auto &mjt = mt->majortype;
  auto &subt = mt->subtype;
  subt;
  auto is_fixed = mt->bFixedSizeSamples;
  is_fixed;
  auto is_zipped = mt->bTemporalCompression;
  is_zipped;
  auto &fmt = mt->formattype;
  fmt;
  wchar_t ids[40];
  auto hr = StringFromGUID2(mjt, ids, sizeof(ids) / sizeof(wchar_t));
  return hr;
}

HRESULT PrintMediaType(DWORD sidx, DWORD mtidx, ComPtr<IMFMediaType> mt) {
  mtidx;
  mt;
  sidx;
  BOOL zipped = FALSE;
  mt->IsCompressedFormat(&zipped);
  AM_MEDIA_TYPE *rep = nullptr;
  auto hr = MFCreateAMMediaTypeFromMFMediaType(mt.Get(), GUID_NULL, &rep);
  //auto hr = mt->GetRepresentation(AM_MEDIA_TYPE_REPRESENTATION, (void**)&rep);
  if (rep!= nullptr)  PrintMediaTypeDetail(zipped, rep);
  if (rep)
    CoTaskMemFree(rep);
    //mt->FreeRepresentation(AM_MEDIA_TYPE_REPRESENTATION, rep);
  return hr;
}
HRESULT EnumMediaTypes(ComPtr<IMFSourceReader> reader) {
  auto hr = S_OK;
  DWORD sidx = 0, mtidx = 0;
  ComPtr<IMFMediaType> mt;
  while (ok(hr)) {
    hr = reader->GetNativeMediaType(sidx, mtidx, mt.ReleaseAndGetAddressOf());
    if(ok(hr)) PrintMediaType(sidx, mtidx, mt);
    ++sidx;
  }
  return S_OK;
}
HRESULT ReadMedia(ComPtr<IMFSourceReader> reader) {
  //ComPtr<IMFMediaSource> source;
  //auto hr = MediaSourceFromReader(reader, source.ReleaseAndGetAddressOf());
  //DWORD count = 0;
//  if (ok(hr))
//    hr = StreamCountFromMediaSource(source, &count);

  //if (ok(hr))
    //hr = PrintMediaTypes(reader, count);
  auto  hr = EnumMediaTypes(reader);

  return hr;
}
HRESULT tmain_imp(int argc, _TCHAR* argv[]) {
  argc; argv;
  auto url = LR"(file:///f:\fs.ts)";
  ComPtr<IMFSourceReader> reader;
  auto hr = MFCreateSourceReaderFromURL(url, NULL, reader.ReleaseAndGetAddressOf());
  if (ok(hr))
    hr = ReadMedia(reader);
  return hr;
}
int _tmain(int argc, _TCHAR* argv[])
{
  auto hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (ok(hr)) hr = MFStartup(MF_VERSION);
  if (ok(hr)) hr = tmain_imp(argc, argv);
  else return hr;
  MFShutdown();
  CoUninitialize();
	return hr;
}
