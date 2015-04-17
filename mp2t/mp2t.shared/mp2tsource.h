// -*- C++ -*-
#pragma once
// Common sample files.
#include "linklist.h"

#include "asynccb.h"
#include "opqueue.h"
#include "critsec.h"
#include "mp2t.h"
// Forward declares
namespace ABI { namespace mp2t { class Mp2tByteStreamHandler; } }

class Mp2tSource;
class Mp2tStream;
class SourceOp;

typedef ComPtrList<IMFSample>       SampleList;
typedef ComPtrList<IUnknown, true>  TokenList;    // List of tokens for IMFMediaStream::RequestSample

enum SourceState {
  STATE_INVALID,      // Initial state. Have not started opening the stream.
  STATE_OPENING,      // BeginOpen is in progress.
  STATE_STOPPED,
  STATE_PAUSED,
  STATE_STARTED,
  STATE_SHUTDOWN
};

//#include "mp2tparse.h"          // MPEG-1 parser
#include "mp2tstream.h"    // MPEG-1 stream

// Constants

const DWORD INITIAL_BUFFER_SIZE = 4 * 1024; // Initial size of the read buffer. (The buffer expands dynamically.)
const DWORD READ_SIZE = 4 * 1024;           // Size of each read request.
const DWORD SAMPLE_QUEUE = 2;               // How many samples does each stream try to hold in its queue?

// Represents a request for an asynchronous operation.

class SourceOp : public IUnknown {
public:

  enum Operation {
    OP_START,
    OP_PAUSE,
    OP_STOP,
    OP_SETRATE,
    OP_REQUEST_DATA,
    OP_END_OF_STREAM
  };

  static HRESULT CreateOp(Operation op, SourceOp **ppOp);
  static HRESULT CreateStartOp(IMFPresentationDescriptor *pPD, SourceOp **ppOp);
  static HRESULT CreateSetRateOp(BOOL fThin, float flRate, SourceOp **ppOp);

  // IUnknown
  STDMETHODIMP QueryInterface(REFIID iid, void **ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  SourceOp(Operation op);
  virtual ~SourceOp();

  HRESULT SetData(const PROPVARIANT &var);

  Operation Op() const { return m_op; }
  const PROPVARIANT &Data() { return m_data; }

protected:
  long        m_cRef;     // Reference count.
  Operation   m_op;
  PROPVARIANT m_data;     // Data for the operation.
};

class StartOp WrlSealed : public SourceOp {
public:
  StartOp(IMFPresentationDescriptor *pPD);
  ~StartOp();

  HRESULT GetPresentationDescriptor(IMFPresentationDescriptor **ppPD);

protected:
  ComPtr<IMFPresentationDescriptor> m_spPD; // Presentation descriptor for Start operations.
};

class SetRateOp WrlSealed : public SourceOp {
public:
  SetRateOp(BOOL fThin, float flRate);
  ~SetRateOp();

  BOOL IsThin() const { return m_fThin; }
  float GetRate() const { return m_flRate; }

private:
  BOOL m_fThin;
  float m_flRate;
};
//[Guid("DEC79B79-E78D-4D4A-9398-C522B3476FB0")]
struct __declspec(uuid("DEC79B79-E78D-4D4A-9398-C522B3476FB0")) __declspec(novtable) ISourceLocker : IUnknown {
  virtual HRESULT STDMETHODCALLTYPE Lock() = 0;
  virtual HRESULT STDMETHODCALLTYPE Unlock() = 0;
};

struct LockerLock {
  ComPtr<ISourceLocker> locker;
  LockerLock(ComPtr<ISourceLocker> locker) :locker(locker) { locker->Lock(); }
  ~LockerLock() { locker->Unlock(); }
};
// CMPEG1Source: The media source object.
class Mp2tSource : public RuntimeClass<RuntimeClassFlags<ClassicCom>
  , IMFMediaSource
  , IMFGetService
  //  , IMFMediaEventGenerator
  , IMFRateControl, ISourceLocker>,
  public OpQueue<Mp2tSource, SourceOp> {
public:// ISourceLocker
  STDMETHODIMP Lock();
  STDMETHODIMP Unlock();
public:
  // IMFMediaEventGenerator
  STDMETHODIMP BeginGetEvent(IMFAsyncCallback *pCallback, IUnknown *punkState);
  STDMETHODIMP EndGetEvent(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent);
  STDMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent);
  STDMETHODIMP QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT *pvValue);

  // IMFMediaSource
  STDMETHODIMP CreatePresentationDescriptor(IMFPresentationDescriptor **ppPresentationDescriptor);
  STDMETHODIMP GetCharacteristics(DWORD *pdwCharacteristics);
  STDMETHODIMP Pause();
  STDMETHODIMP Shutdown();
  STDMETHODIMP Start(
    IMFPresentationDescriptor *pPresentationDescriptor,
    const GUID *pguidTimeFormat,
    const PROPVARIANT *pvarStartPosition
    );
  STDMETHODIMP Stop();

  // IMFGetService
  IFACEMETHOD(GetService) (_In_ REFGUID guidService, _In_ REFIID riid, _Out_opt_ LPVOID *ppvObject);

  // IMFRateControl
  IFACEMETHOD(SetRate) (BOOL fThin, float flRate);
  IFACEMETHOD(GetRate) (_Inout_opt_ BOOL *pfThin, _Inout_opt_ float *pflRate);

  // Called by the byte stream handler.
  concurrency::task<void> OpenAsync(IMFByteStream *pStream);

  // Queues an asynchronous operation, specify by op-type.
  // (This method is public because the streams call it.)
  HRESULT QueueAsyncOperation(SourceOp::Operation OpType);

  // Callbacks
  HRESULT OnByteStreamRead(IMFAsyncResult *pResult);  // Async callback for RequestData
public:
  Mp2tSource();
  ~Mp2tSource();

private:

  // CheckShutdown: Returns MF_E_SHUTDOWN if the source was shut down.
  HRESULT CheckShutdown() const {
    return (m_state == STATE_SHUTDOWN ? MF_E_SHUTDOWN : S_OK);
  }

  void        CompleteOpen(HRESULT hrStatus);

  HRESULT     IsInitialized() const;
  bool        IsStreamTypeSupported(StreamType type) const;
  bool        IsStreamActive(const Mp2tHeader &packetHdr);
  bool        StreamsNeedData() const;

  void        DoStart(StartOp *pOp);
  void        DoStop(SourceOp *pOp);
  void        DoPause(SourceOp *pOp);
  void        DoSetRate(SourceOp *pOp);
  void        OnStreamRequestSample(SourceOp *pOp);
  void        OnEndOfStream(SourceOp *pOp);

  void        InitPresentationDescriptor();
  void        SelectStreams(IMFPresentationDescriptor *pPD, const PROPVARIANT varStart);

  void        RequestData(DWORD cbRequest);
  void        ParseData();
  bool        ReadPayload(DWORD *pcbAte, DWORD *pcbNextRequest);
  void        DeliverPayload();
  void        EndOfMPEGStream();

  void        CreateStream(const Mp2tHeader &packetHdr);

  HRESULT     ValidatePresentationDescriptor(IMFPresentationDescriptor *pPD);

  // Handler for async errors.
  void        StreamingError(HRESULT hr);

  void        BeginAsyncOp(SourceOp *pOp);
  void        CompleteAsyncOp(SourceOp *pOp);
  HRESULT     DispatchOperation(SourceOp *pOp);
  HRESULT     ValidateOperation(SourceOp *pOp);

  bool        IsRateSupported(float flRate, float *pflAdjustedRate);

private:
  long                        m_cRef;                     // reference count

  CritSec                     m_critSec;                  // critical section for thread safety
  SourceState                 m_state;                    // Current state (running, stopped, paused)

//  Buffer                      ^m_ReadBuffer;
//  Parser                      ^m_parser;

  ComPtr<IMFMediaEventQueue>  m_spEventQueue;             // Event generator helper
  ComPtr<IMFPresentationDescriptor> m_spPresentationDescriptor; // Presentation descriptor.
  concurrency::task_completion_event<void> _openedEvent;  // Event used to signalize end of open operation.
  ComPtr<IMFByteStream>       m_spByteStream;

  //ExpandableStruct<Mp2tHeader> ^m_header;
  Mp2tHeader                  m_header;
  StreamVector                m_streams;                  // Array of streams.

  DWORD                       m_cPendingEOS;              // Pending EOS notifications.
  ULONG                       m_cRestartCounter;          // Counter for sample requests.

  ComPtr<SourceOp>            m_spCurrentOp;
  ComPtr<SourceOp>            m_spSampleRequest;

  // Async callback helper.
  AsyncCallback<Mp2tSource>  m_OnByteStreamRead;

  float                       m_flRate;
};
