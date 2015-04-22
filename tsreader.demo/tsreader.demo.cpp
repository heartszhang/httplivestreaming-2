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
  auto zipped = FALSE;
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
struct Lock {
  Lock() { OutputDebugStringW( L"enter Lock::ctor\n" ); }
  ~Lock() { OutputDebugStringW( L"exit Lock::dtor\n" ); }
};
HRESULT tmain_imp(int argc, _TCHAR* argv[]) {
  Lock (); 
  argc; argv;
  //auto url = LR"(http://gslb.bestvcdn.com.cloudcdn.net/218.77.90.60/gslb/program/Dbackct_bestvcdn_comD/_9Xxr00bTIO_/FDN/FDNB1690862/700/stream.m3u8?_cp=1&_back=FASTWEB&taskID=pb02.idc.xbox.bestv.com.cn_1428984407481_023513000004901)";
  auto url = LR"(http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8)";
  ComPtr<IMFSourceReader> reader;
  auto hr = MFCreateSourceReaderFromURL(url, nullptr, reader.ReleaseAndGetAddressOf());
  if (ok(hr))
    hr = ReadMedia(reader);
  return hr;
}
int _tmain(int argc, _TCHAR* argv[])
{
  auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (ok(hr)) hr = MFStartup(MF_VERSION);
  if (ok(hr)) hr = tmain_imp(argc, argv);
  MFShutdown();
  CoUninitialize();
  return hr;
}
