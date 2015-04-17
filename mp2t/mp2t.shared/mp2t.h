#pragma once
#include <linklist.h>
#include <vector>
#include <cassert>
enum StreamType {
  StreamType_Unknown,
  StreamType_Video,
  StreamType_Audio
};
enum SourceState {
  STATE_INVALID,      // Initial state. Have not started opening the stream.
  STATE_OPENING,      // BeginOpen is in progress.
  STATE_STOPPED,
  STATE_PAUSED,
  STATE_STARTED,
  STATE_SHUTDOWN
};

class Mp2tStream;

class StreamVector {
  std::vector<ComPtr<Mp2tStream>> streams;
  std::vector<BYTE>               ids;
public:
  ~StreamVector();
  size_t GetCount()const;
  void Clear();

  HRESULT AddStream(BYTE id, Mp2tStream *pStream);
  ComPtr<Mp2tStream> Find(BYTE id)const;

  ComPtr<Mp2tStream>& operator[](DWORD index);
  const ComPtr<Mp2tStream>& operator[](DWORD index) const;
};
using SampleVector = ComVector<IMFSample>;
using TokenVector = ComVector<IUnknown>;

struct Mp2tHeader {
  BYTE          stream_id = 0;
  StreamType    type = StreamType_Unknown;
  DWORD         cStreams = 0;
  StreamVector  streams;
};
