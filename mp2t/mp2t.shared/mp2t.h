#pragma once
#include <vector>
#include <cassert>
enum StreamType {
  StreamType_Unknown,
  StreamType_Video,
  StreamType_Audio
};
class Mp2tStream;

class StreamVector {
  const static DWORD    MAX_STREAMS = 32;
  //  ComPtr<Mp2tStream>    streams[MAX_STREAMS];
//  BYTE                  ids[MAX_STREAMS];
//  DWORD                 count = 0;
  std::vector<ComPtr<Mp2tStream>> streams;
  std::vector<BYTE>               ids;
public:
  ~StreamVector() {
    Clear();
  }
  size_t GetCount()const { return streams.size(); }
  void Clear() {
    streams.clear();
    for (DWORD i = 0; i < MAX_STREAMS; i++) {
      streams[i].Reset();
    }
    //    count = 0;
  }

  HRESULT AddStream(BYTE id, Mp2tStream *pStream) {
    streams.push_back(ComPtr<Mp2tStream>(pStream));
    //streams[count] = pStream;
    //ids[count] = id;
    ids.push_back(id);
    //++count;

    return S_OK;
  }

  ComPtr<Mp2tStream> Find(BYTE id) {
    // This method can return nullptr if the source did not create a
    // stream for this ID. In particular, this can happen if:
    //
    // 1) The stream type is not supported. See IsStreamTypeSupported().
    // 2) The source is still opening.
    //
    // Note: This method does not AddRef the stream object. The source
    // uses this method to access the streams. If the source hands out
    // a stream pointer (e.g. in the MENewStream event), the source
    // must AddRef the stream object.
    for (auto i = 0; i < streams.size(); i++) {
      if (ids[i] == id) {
        return streams[i];
      }
    }
    return ComPtr<Mp2tStream>();
    /*
    Mp2tStream *pStream = nullptr;
    for (UINT32 i = 0; i < count; i++) {
      if (ids[i] == id) {
        pStream = streams[i].Get();
        break;
      }
    }
    return pStream;
    */
  }

  // Accessor.
  ComPtr<Mp2tStream> operator[](DWORD index) {
    assert(index < count);
    return streams[index].Get();
  }

  // Const accessor.
  Mp2tStream *const operator[](DWORD index) const {
    assert(index < count);
    return streams[index].Get();
  }
};

struct Mp2tHeader {
  BYTE          stream_id = 0;
  StreamType    type = StreamType_Unknown;
  DWORD         cStreams = 0;
  StreamVector  streams;
};
