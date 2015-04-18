// -*- C++ -*-
#pragma once
// Common sample files.
#include "mp2t.h"

#include "opqueue.h"
// Forward declares
namespace ABI { namespace mp2t { class Mp2tByteStreamHandler; } }

class Mp2tSource;
class Mp2tStream;
class SourceOp;

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
#if 0
class StartOp WrlSealed : public SourceOp {
public:
  StartOp(IMFPresentationDescriptor *pPD);
  ~StartOp();

  HRESULT GetPresentationDescriptor(IMFPresentationDescriptor **ppPD);

protected:
  ComPtr<IMFPresentationDescriptor> m_spPD; // Presentation descriptor for Start operations.
};
#endif
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
// CMPEG1Source: The media source object.
class Mp2tSource : public RuntimeClass<RuntimeClassFlags<ClassicCom>
  , IMFMediaSource
  , IMFGetService
  //  , IMFMediaEventGenerator
  , IMFRateControl, ISourceLocker> {
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
  HRESULT BeginRequestData(BYTE *buf, ULONG blen, IMFAsyncCallback*cb, IUnknown*stat = nullptr);
  HRESULT EndRequestData(IMFAsyncResult *, ULONG *readed);
  HRESULT OnByteStreamRead(IMFMediaBuffer *pResult);  // Async callback for RequestData
public:
  Mp2tSource();
  ~Mp2tSource();

private:

  // CheckShutdown: Returns MF_E_SHUTDOWN if the source was shut down.
  HRESULT CheckShutdown() const {
    return (_state == STATE_SHUTDOWN ? MF_E_SHUTDOWN : S_OK);
  }

  void        CompleteOpen(HRESULT hrStatus);

  HRESULT     IsInitialized() const;
  bool        IsStreamTypeSupported(StreamType type) const;
  bool        IsStreamActive(const Mp2tHeader &packetHdr);
  bool        StreamsNeedData() const;

  HRESULT        DoStart(IMFPresentationDescriptor *pd, PROPVARIANT const*start_time);
  HRESULT        DoStop();
  HRESULT        DoPause();
  HRESULT        DoSetRate(float r);
  void        OnStreamRequestSample(SourceOp *pOp);
  void        OnEndOfStream(SourceOp *pOp);

  void        InitPresentationDescriptor();
  HRESULT        SelectStreams(IMFPresentationDescriptor *pPD, const PROPVARIANT *varStart);

  void        RequestData(DWORD cbRequest);
  void        ParseData();
  bool        ReadPayload(DWORD *pcbAte, DWORD *pcbNextRequest);
  void        DeliverPayload();
  void        EndOfMPEGStream();

  void        CreateStream(const Mp2tHeader &packetHdr);

  HRESULT     ValidatePresentationDescriptor(IMFPresentationDescriptor *pPD);

  // Handler for async errors.
  void        StreamingError(HRESULT hr);

  //  void        BeginAsyncOp(SourceOp *pOp);
  //  void        CompleteAsyncOp(SourceOp *pOp);
  HRESULT     DispatchOperation(SourceOp *pOp);
  HRESULT     ValidateOperation(SourceOp *pOp);

  bool        IsRateSupported(float flRate, float *pflAdjustedRate);

private:
  ::Lock                     _lock;                  // critical section for thread safety
  SourceState                _state;                 // Current state (running, stopped, paused)

//  Buffer                      ^m_ReadBuffer;
//  Parser                      ^m_parser;

  ComPtr<IMFMediaEventQueue>  _event_q;             // Event generator helper
  ComPtr<IMFPresentationDescriptor> _pd; // Presentation descriptor.
  concurrency::task_completion_event<void> _openedEvent;  // Event used to signalize end of open operation.
  ComPtr<IMFByteStream>       _byte_stream;

  //ExpandableStruct<Mp2tHeader> ^m_header;
  Mp2tHeader                  m_header;
  StreamVector                _streams;                  // Array of streams.

  DWORD                       m_cPendingEOS;              // Pending EOS notifications.
  ULONG                       m_cRestartCounter;          // Counter for sample requests.

//  ComPtr<SourceOp>            m_spCurrentOp;
  ComPtr<SourceOp>            m_spSampleRequest;

  // Async callback helper.
//  ComPtr<IMFAsyncCallback>  m_OnByteStreamRead;

  float                       _rate = 1.0f;
};
