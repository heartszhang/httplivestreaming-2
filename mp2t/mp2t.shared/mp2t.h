#pragma once
#include <vector>
#include <functional>
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

struct Mp2tHeader {
  BYTE          stream_id = 0;
  StreamType    type = StreamType_Unknown;
  DWORD         cStreams = 0;
  StreamVector  streams;
};
template<typename T, typename F>
ComPtr<T> Cast(ComPtr<F> i) {
  auto r = ComPtr<T>();
  i.As(&r);  // ignore hresult
  return r;
}
using InvokeFun = std::function<HRESULT(IMFAsyncResult*)>;
auto CreateAsyncCallback(InvokeFun func)->ComPtr < IMFAsyncCallback >;
auto CreateBuffer(DWORD len)->ComPtr<IMFMediaBuffer>;
template<typename I> using ComVector = std::vector<ComPtr<I>>;
using SampleVector = ComVector<IMFSample>;
using TokenVector = ComVector<IUnknown>;

//[Guid("DEC79B79-E78D-4D4A-9398-C522B3476FB0")]
struct __declspec(uuid("DEC79B79-E78D-4D4A-9398-C522B3476FB0")) __declspec(novtable) ISourceLocker : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE Lock() = 0;
  virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
};

struct Locker {
  ComPtr<ISourceLocker> locker;
  Locker(ComPtr<ISourceLocker> locker) :locker(locker) { locker->Lock(); }
  ~Locker() { locker->Unlock(); }
};

struct Lock : CRITICAL_SECTION
{
public:
  Lock() { InitializeCriticalSectionEx(this, 100, 0); } // why 100?
  ~Lock() { DeleteCriticalSection(this); }

  void lock() { EnterCriticalSection(this); }// different with ctor
  void unlock() { LeaveCriticalSection(this); }
};

inline bool ok(HRESULT hr) { return SUCCEEDED(hr); }
inline bool failed(HRESULT hr) { return FAILED(hr); }

struct PropVar : PROPVARIANT {
  PropVar() { PropVariantInit(this); }
  ~PropVar() { PropVariantClear(this); }
  //PropVar& operator= (PropVar const&) {}
  explicit PropVar(PROPVARIANT const*v) { Copy(v); }
  void Copy(PROPVARIANT const*v) { PropVariantCopy(this, v); }
};

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
