// TsReaderDemo.cpp : Defines the entry point for the console application.
//

#include "pch.h"
#include <strmif.h> // AM_MEDIA_TYPE
#include "m3u8.h"
/*
This structure represents a single bitrate playlist aka media playlist.
It related to both a simple media playlists and a sliding window media playlists.
URI lines in the Playlist point to media segments.
Simple Media Playlist file sample:
*/
const auto medialist_sample = R"(#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:5220
#EXTINF:5219.2,
http://media.example.com/entire.ts
#EXT-X-ENDLIST
)";
//Sample of Sliding Window Media Playlist, using HTTPS:
const auto live_sample = R"(
#EXTM3U
#EXT-X-VERSION:3
#EXT-X-TARGETDURATION:8
#EXT-X-MEDIA-SEQUENCE:2680
#EXTINF:7.975,
https://priv.example.com/fileSequence2680.ts
#EXTINF:7.941,
https://priv.example.com/fileSequence2681.ts
#EXTINF:7.975,
https://priv.example.com/fileSequence2682.ts
)";

/*
This structure represents a master playlist which combines media playlists for multiple bitrates.
URI lines in the playlist identify media playlists.
Sample of Master Playlist file:
*/
auto masterlist_sample = R"(
#EXTM3U
#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=1280000
http://example.com/low.m3u8
#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=2560000
http://example.com/mid.m3u8
#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=7680000
http://example.com/hi.m3u8
#EXT-X-STREAM-INF:PROGRAM-ID=1,BANDWIDTH=65000,CODECS="mp4a.40.5"
http://example.com/audio-only.m3u8
)";

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
using namespace m3u8;
HRESULT tmain_imp( int argc, _TCHAR* argv[] ) {
  argc; argv;
  auto reader = new buffer_reader( medialist_sample );
  auto r = decode_playlist(reader);
  r;
  delete reader;
  return S_OK;
}
HRESULT tmain_imp2( int argc, _TCHAR* argv[] ) {
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
