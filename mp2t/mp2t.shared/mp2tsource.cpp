#include "pch.h"
#include "mp2tsource.h"

ComPtr<IMFMediaType> CreateVideoMediaType(const Mp2tHeader &videoSeqHdr);
ComPtr<IMFMediaType> CreateAudioMediaType(const Mp2tHeader &audioHeader);
void GetStreamMajorType(IMFStreamDescriptor *pSD, GUID *pguidMajorType);

STDMETHODIMP Mp2tSource::Lock() {
  _lock.lock();
  return S_OK;
}

STDMETHODIMP Mp2tSource::Unlock() {
  _lock.unlock();
  return S_OK;
}

HRESULT Mp2tSource::BeginGetEvent(IMFAsyncCallback *pCallback, IUnknown *punkState) {
  Locker lock(this);

  auto hr = CheckShutdown();

  if (ok(hr))
    hr = _event_q->BeginGetEvent(pCallback, punkState);
  return hr;
}

HRESULT Mp2tSource::EndGetEvent(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent) {
  Locker lock(this);
  HRESULT hr = _event_q->EndGetEvent(pResult, ppEvent);

  if (ok(hr))
    hr = CheckShutdown();

  return hr;
}

HRESULT Mp2tSource::GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent) {
  auto hr = S_OK;
  ComPtr<IMFMediaEventQueue> q;

  {
    Locker lock(this);
    q = _event_q;
    hr = CheckShutdown();
  }

  // Now get the event.
  if (ok(hr))
    hr = q->GetEvent(dwFlags, ppEvent);

  return hr;
}

HRESULT Mp2tSource::QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT *pvValue) {
  Locker lock(this);

  auto hr = CheckShutdown();

  if (ok(hr))
    hr = _event_q->QueueEventParamVar(met, guidExtendedType, hrStatus, pvValue);

  return hr;
}

HRESULT Mp2tSource::CreatePresentationDescriptor(IMFPresentationDescriptor **pd) {
  if (pd == nullptr)
    return E_POINTER;
  Locker lock(this);

  if (!_pd)
    return MF_E_NOT_INITIALIZED;

  auto hr = CheckShutdown();

  // Fail if the source was not initialized yet.
  if (ok(hr))
    hr = IsInitialized();

  if (ok(hr))
    hr = _pd->Clone(pd);

  return hr;
}

HRESULT Mp2tSource::GetCharacteristics(DWORD *c) {
  if (c == nullptr)
    return E_POINTER;

  Locker lock(this);

  auto hr = CheckShutdown();

  if (ok(hr))
    *c = MFMEDIASOURCE_CAN_PAUSE | MFMEDIASOURCE_CAN_SEEK | MFMEDIASOURCE_HAS_SLOW_SEEK | MFMEDIASOURCE_CAN_SKIPFORWARD | MFMEDIASOURCE_CAN_SKIPBACKWARD;

  return hr;
}

HRESULT Mp2tSource::Pause() {
  Locker lock(this);
  ComPtr<Mp2tSource> me(this);
  auto hr = CheckShutdown();

  if (ok(hr))
    hr = Async([me](IMFAsyncResult*)->HRESULT {return me->DoPause(); });
  return hr;
}

HRESULT Mp2tSource::Shutdown() {
  Locker lock(this);
  auto hr = CheckShutdown();
  if (failed(hr))
    return hr;

  Mp2tStream *pStream = nullptr;
  _state = STATE_SHUTDOWN;

  for (DWORD i = 0; i < _streams.GetCount(); i++) {
    (void)_streams[i]->Shutdown();
  }
  _streams.Clear();

  (void)_event_q->Shutdown();

  _byte_stream = nullptr;
  _event_q = nullptr;
  _streams.Clear();
  return hr;
}

HRESULT Mp2tSource::Start(
  IMFPresentationDescriptor *pd,
  const GUID *pguidTimeFormat,
  const PROPVARIANT *pvarStartPos) {
  // Start position and presentation descriptor cannot be nullptr.
  if (pvarStartPos == nullptr || pd == nullptr)
    return E_INVALIDARG;

  // Check the time format.
  if ((pguidTimeFormat != nullptr) && (*pguidTimeFormat != GUID_NULL))
    // Unrecognized time format GUID.
    return MF_E_UNSUPPORTED_TIME_FORMAT;

  // Check the data type of the start position.
  if ((pvarStartPos->vt != VT_I8) && (pvarStartPos->vt != VT_EMPTY))
    return MF_E_UNSUPPORTED_TIME_FORMAT;

  //ComPtr<SourceOp> spAsyncOp;

  Locker lock(this);

  // Check if this is a seek request. This sample does not support seeking.
  if (pvarStartPos->vt == VT_I8 && ((_state != STATE_STOPPED) || (pvarStartPos->hVal.QuadPart != 0)))
    return MF_E_INVALIDREQUEST;

  // Fail if the source is shut down.
  auto hr = CheckShutdown();
  // Fail if the source was not initialized yet.
  if (ok(hr)) hr = IsInitialized();

  // Perform a sanity check on the caller's presentation descriptor.
  if (ok(hr)) hr = ValidatePresentationDescriptor(pd);

  ComPtr<Mp2tSource> me(this);
  PropVar start_time(pvarStartPos);
  ComPtr<IMFPresentationDescriptor> xpd(pd);
  // The operation looks OK. Complete the operation asynchronously.
  if (ok(hr)) hr = Async([me, start_time, xpd](IMFAsyncResult*)->HRESULT {
    return me->DoStart(xpd.Get(), &start_time);
  });

  return hr;
}

HRESULT Mp2tSource::Stop() {
  Locker lock(this);

  auto hr = CheckShutdown();
  // Fail if the source was not initialized yet.
  if (ok(hr))     hr = IsInitialized();

  // Queue the operation.
  ComPtr<Mp2tSource> me(this);
  if (ok(hr))
    hr = Async([me](IMFAsyncResult*)->HRESULT {return me->DoStop(); });
  return hr;
}

HRESULT Mp2tSource::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject) {
  HRESULT hr = MF_E_UNSUPPORTED_SERVICE;

  if (ppvObject == nullptr)
    return E_POINTER;

  if (guidService == MF_RATE_CONTROL_SERVICE)
    hr = QueryInterface(riid, ppvObject);

  return hr;
}

// IMFRateControl methods
// Sets a rate on the source. Only supported rates are 0 and 1.
HRESULT Mp2tSource::SetRate(BOOL fThin, float flRate) {
  if (flRate == _rate)
    return S_OK;
  if (fThin)
    return MF_E_THINNING_UNSUPPORTED;

  if (!IsRateSupported(flRate, &flRate))
    return MF_E_UNSUPPORTED_RATE;

  Locker lock(this);
  ComPtr<Mp2tSource> me(this);
  auto hr = Async([me, flRate](IMFAsyncResult*)->HRESULT {return me->DoSetRate(flRate); });

  return hr;
}

HRESULT Mp2tSource::GetRate(BOOL *pfThin, float *pflRate) {
  if (pfThin == nullptr || pflRate == nullptr)
    return E_INVALIDARG;

  Locker lock(this);
  *pfThin = FALSE;
  *pflRate = _rate;

  return S_OK;
}

HRESULT Mp2tSource::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample * pSample)
{
  return E_NOTIMPL;
}

HRESULT Mp2tSource::OnFlush(DWORD dwStreamIndex)
{
  return E_NOTIMPL;
}

HRESULT Mp2tSource::OnEvent(DWORD dwStreamIndex, IMFMediaEvent * pEvent)
{
  return E_NOTIMPL;
}

//-------------------------------------------------------------------
// Public non-interface methods
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// BeginOpen
// Begins reading the byte stream to initialize the source.
// Called by the byte-stream handler when it creates the source.
//
// This method is asynchronous. When the operation completes,
// the callback is invoked and the byte-stream handler calls
// EndOpen.
//
// pStream: Pointer to the byte stream for the MPEG-1 stream.
// pCB: Pointer to the byte-stream handler's callback.
// pState: State object for the async callback. (Can be nullptr.)
//
// Note: The source reads enough data to find one packet header
// for each audio or video stream. This enables the source to
// create a presentation descriptor that describes the format of
// each stream. The source queues the packets that it reads during
// BeginOpen.
//-------------------------------------------------------------------

HRESULT Mp2tSource::BeginOpen(IMFByteStream *s, IMFAsyncCallback*cb, IUnknown*stat) {
  if (s == nullptr)
    return E_INVALIDARG;
  Locker lock(this);
  if (_state != STATE_STARTED)
    return MF_E_INVALIDREQUEST;
  auto hr = MFCreateEventQueue(_event_q.ReleaseAndGetAddressOf());
  _byte_stream = s;
  DWORD cap = 0;
  if (ok(hr))
    hr = _byte_stream->GetCapabilities(&cap);
  if (ok(hr) && (cap & MFBYTESTREAM_IS_READABLE) == 0)
    hr = MF_E_UNSUPPORTED_BYTESTREAM_TYPE;
  if (ok(hr) && (cap & MFBYTESTREAM_IS_SEEKABLE) == 0)
    hr = MF_E_BYTESTREAM_NOT_SEEKABLE;

  if (failed(hr))
    return hr;
  _state = STATE_OPENING;
  ComPtr<Mp2tSource> me(this);
  auto xbuf = CreateBuffer(INITIAL_BUFFER_SIZE);
  BYTE*buf = nullptr;
  ULONG blen = 0;
  hr = xbuf->Lock(&buf, &blen, nullptr);
  auto xcb = CreateAsyncCallback([xbuf, me](IMFAsyncResult*result) ->HRESULT {
    ULONG readed = 0;
    auto hr = me->EndRequestData(result, &readed);
    if (ok(hr))
      hr = xbuf->SetCurrentLength(readed);
    // todo: do what here
    xbuf->Unlock();
    return hr;
  });
  if (ok(hr))
    hr = BeginRequestData(buf, blen, xcb.Get(), nullptr);
  return hr;
#if 0
  if (pStream == nullptr) {
    throw ref new InvalidArgumentException();
  }

  if (m_state != STATE_INVALID) {
    ThrowException(MF_E_INVALIDREQUEST);
  }

  DWORD dwCaps = 0;

  AutoLock lock(m_critSec);

  // Create the media event queue.
  ThrowIfError(MFCreateEventQueue(m_spEventQueue.ReleaseAndGetAddressOf()));

  // Cache the byte-stream pointer.
  m_spByteStream = pStream;

  // Validate the capabilities of the byte stream.
  // The byte stream must be readable and seekable.
  ThrowIfError(pStream->GetCapabilities(&dwCaps));

  if ((dwCaps & MFBYTESTREAM_IS_SEEKABLE) == 0) {
    ThrowException(MF_E_BYTESTREAM_NOT_SEEKABLE);
  }
  else if ((dwCaps & MFBYTESTREAM_IS_READABLE) == 0) {
    ThrowException(MF_E_UNSUPPORTED_BYTESTREAM_TYPE);
  }

  // Reserve space in the read buffer.
  m_ReadBuffer = ref new Buffer(INITIAL_BUFFER_SIZE);

  // Create the MPEG-1 parser.
  m_parser = ref new Parser();

  RequestData(READ_SIZE);

  m_state = STATE_OPENING;
  return concurrency::create_task(_openedEvent);

#endif
}

//-------------------------------------------------------------------
// OnByteStreamRead
// Called when an asynchronous read completes.
//
// Read requests are issued in the RequestData() method.
//-------------------------------------------------------------------

HRESULT Mp2tSource::OnByteStreamRead(IMFMediaBuffer *pResult) {
#if 0
  AutoLock lock(m_critSec);

  DWORD cbRead = 0;

  IUnknown *pState = nullptr;

  if (m_state == STATE_SHUTDOWN) {
    // If we are shut down, then we've already released the
    // byte stream. Nothing to do.
    return S_OK;
  }

  try {
    // Get the state object. This is either nullptr or the most
    // recent OP_REQUEST_DATA operation.
    (void)pResult->GetState(&pState);

    // Complete the read opertation.
    ThrowIfError(m_spByteStream->EndRead(pResult, &cbRead));

    // If the source stops and restarts in rapid succession, there is
    // a chance this is a "stale" read request, initiated before the
    // stop/restart.

    // To ensure that we don't deliver stale data, we store the
    // OP_REQUEST_DATA operation as a state object in pResult, and compare
    // this against the current value of m_cRestartCounter.

    // If they don't match, we discard the data.

    // NOTE: During BeginOpen, pState is nullptr

    if ((pState == nullptr) || (((SourceOp*)pState)->Data().ulVal == m_cRestartCounter)) {
      // This data is OK to parse.

      if (cbRead == 0) {
        // There is no more data in the stream. Signal end-of-stream.
        EndOfMPEGStream();
      }
      else {
        // Update the end-position of the read buffer.
        m_ReadBuffer->MoveEnd(cbRead);

        // Parse the new data.
        ParseData();
      }
    }
  }
  catch (Exception ^exc) {
    StreamingError(exc->HResult);
  }
#endif
  return S_OK;
}

/* Private methods */

Mp2tSource::Mp2tSource() :
  _state(STATE_INVALID),
  m_cRestartCounter(0),
  _rate(1.0f) {
  //  m_OnByteStreamRead = CreateAsyncCallback([this](IMFAsyncResult*result) ->HRESULT {
  //    return this->OnByteStreamRead(result);
  //  });
}

Mp2tSource::~Mp2tSource() {
  if (_state != STATE_SHUTDOWN) {
    Shutdown();
  }
}

//-------------------------------------------------------------------
// CompleteOpen
//
// Completes the asynchronous BeginOpen operation.
//
// hrStatus: Status of the BeginOpen operation.
//-------------------------------------------------------------------

void Mp2tSource::CompleteOpen(HRESULT hrStatus) {
  assert(!_openedEvent._IsTriggered());
  if (FAILED(hrStatus)) {
    Shutdown();
    //    _openedEvent.set_exception(ref new COMException(hrStatus));
  }
  else {
    _openedEvent.set();
  }
}

//-------------------------------------------------------------------
// IsInitialized:
// Returns S_OK if the source is correctly initialized with an
// MPEG-1 byte stream. Otherwise, returns MF_E_NOT_INITIALIZED.
//-------------------------------------------------------------------

HRESULT Mp2tSource::IsInitialized() const {
  if (_state == STATE_OPENING || _state == STATE_INVALID) {
    return MF_E_NOT_INITIALIZED;
  }
  else {
    return S_OK;
  }
}

//-------------------------------------------------------------------
// IsStreamTypeSupported:
// Returns TRUE if the source supports the specified MPEG-1 stream
// type.
//-------------------------------------------------------------------

bool Mp2tSource::IsStreamTypeSupported(StreamType type) const {
  // We support audio and video streams.
  return (type == StreamType_Video || type == StreamType_Audio);
}

//-------------------------------------------------------------------
// IsStreamActive:
// Returns TRUE if the source should deliver a payload, whose type
// is indicated by the specified packet header.
//
// Note: This method does not test the started/paused/stopped state
//       of the source.
//-------------------------------------------------------------------

bool Mp2tSource::IsStreamActive(const Mp2tHeader &packetHdr) {
  if (_state == STATE_OPENING) {
    // The source is still opening.
    // Deliver payloads for every supported stream type.
    return IsStreamTypeSupported(packetHdr.type);
  }
  else {
    // The source is already opened. Check if the stream is active.
    auto wpStream = _streams.Find(packetHdr.stream_id);

    if (wpStream == nullptr) {
      return false;
    }
    else {
      return wpStream->IsActive();
    }
  }
}

//-------------------------------------------------------------------
// InitPresentationDescriptor
//
// Creates the source's presentation descriptor, if possible.
//
// During the BeginOpen operation, the source reads packets looking
// for headers for each stream. This enables the source to create the
// presentation descriptor, which describes the stream formats.
//
// This method tests whether the source has seen enough packets
// to create the PD. If so, it invokes the callback to complete
// the BeginOpen operation.
//-------------------------------------------------------------------

void Mp2tSource::InitPresentationDescriptor() {
#if 0
  DWORD cStreams = 0;

  assert(m_spPresentationDescriptor == nullptr);
  assert(m_state == STATE_OPENING);
  if (m_header == nullptr) {
    ThrowException(MF_E_MEDIA_SOURCE_NOT_STARTED);
  }
  // Get the number of streams, as declared in the MPEG-1 header, skipping
  // any streams with an unsupported format.
  for (DWORD i = 0; i < m_header.cStreams; i++) {
    if (IsStreamTypeSupported(m_header.streams[i].type)) {
      cStreams++;
    }
  }

  // How many streams do we actually have?
  if (cStreams > m_streams.GetCount()) {
    // Keep reading data until we have seen a packet for each stream.
    return;
  }

  // We should never create a stream we don't support.
  assert(cStreams == m_streams.GetCount());

  // Ready to create the presentation descriptor.

  // Create an array of IMFStreamDescriptor pointers.
  IMFStreamDescriptor **ppSD =
    new (std::nothrow) IMFStreamDescriptor*[cStreams];

  if (ppSD == nullptr) {
    throw ref new OutOfMemoryException();
  }

  ZeroMemory(ppSD, cStreams * sizeof(IMFStreamDescriptor*));

  Exception ^error;
  try {
    // Fill the array by getting the stream descriptors from the streams.
    for (DWORD i = 0; i < cStreams; i++) {
      ThrowIfError(m_streams[i]->GetStreamDescriptor(&ppSD[i]));
    }

    // Create the presentation descriptor.
    ThrowIfError(MFCreatePresentationDescriptor(cStreams, ppSD,
      &m_spPresentationDescriptor));

    // Select the first video stream (if any).
    for (DWORD i = 0; i < cStreams; i++) {
      GUID majorType = GUID_NULL;

      GetStreamMajorType(ppSD[i], &majorType);
      ThrowIfError(m_spPresentationDescriptor->SelectStream(i));
    }

    // Switch state from "opening" to stopped.
    m_state = STATE_STOPPED;

    // Invoke the async callback to complete the BeginOpen operation.
    CompleteOpen(S_OK);
  }
  catch (Exception ^exc) {
    error = exc;
  }

  if (ppSD != nullptr) {
    for (DWORD i = 0; i < cStreams; i++) {
      ppSD[i]->Release();
    }
    delete[] ppSD;
  }

  if (error != nullptr) {
    throw error;
  }
#endif
}

//-------------------------------------------------------------------
// QueueAsyncOperation
// Queue an asynchronous operation.
//
// OpType: Type of operation to queue.
//
// Note: If the SourceOp object requires additional information, call
// OpQueue<SourceOp>::QueueOperation, which takes a SourceOp pointer.
//-------------------------------------------------------------------

HRESULT Mp2tSource::QueueAsyncOperation(SourceOp::Operation OpType) {
  HRESULT hr = S_OK;
  ComPtr<SourceOp> spOp;
  ComPtr<Mp2tSource> me(this);

  hr = SourceOp::CreateOp(OpType, &spOp);

  if (SUCCEEDED(hr)) {
    hr = Async([me, spOp](IMFAsyncResult*)->HRESULT {me->DispatchOperation(spOp.Get()); return S_OK; });
  }
  return hr;
}

//-------------------------------------------------------------------
// BeginAsyncOp
//
// Starts an asynchronous operation. Called by the source at the
// begining of any asynchronous operation.
//-------------------------------------------------------------------
#if 0

void Mp2tSource::BeginAsyncOp(SourceOp *pOp) {
  // At this point, the current operation should be nullptr (the
  // previous operation is nullptr) and the new operation (pOp)
  // must not be nullptr.
  if (pOp == nullptr) {
    throw ref new InvalidArgumentException();
  }

  if (m_spCurrentOp != nullptr) {
    ThrowException(MF_E_INVALIDREQUEST);
  }

  // Store the new operation as the current operation.

  m_spCurrentOp = pOp;
}
#endif

//-------------------------------------------------------------------
// CompleteAsyncOp
//
// Completes an asynchronous operation. Called by the source at the
// end of any asynchronous operation.
//-------------------------------------------------------------------
#if 0

void Mp2tSource::CompleteAsyncOp(SourceOp *pOp) {
  HRESULT hr = S_OK;
  // At this point, the current operation (m_spCurrentOp)
  // must match the operation that is ending (pOp).

  if (pOp == nullptr) {
    throw ref new InvalidArgumentException();
  }

  if (m_spCurrentOp == nullptr) {
    ThrowException(MF_E_INVALIDREQUEST);
  }

  if (m_spCurrentOp.Get() != pOp) {
    throw ref new InvalidArgumentException();
  }

  // Release the current operation.
  m_spCurrentOp.Reset();

  // Process the next operation on the queue.
  ThrowIfError(ProcessQueue());
}
#endif

//-------------------------------------------------------------------
// DispatchOperation
//
// Performs the asynchronous operation indicated by pOp.
//
// NOTE:
// This method implements the pure-virtual OpQueue::DispatchOperation
// method. It is always called from a work-queue thread.
//-------------------------------------------------------------------

HRESULT Mp2tSource::DispatchOperation(SourceOp *pOp) {
#if 0
  AutoLock lock(m_critSec);

  HRESULT hr = S_OK;

  if (m_state == STATE_SHUTDOWN) {
    return S_OK; // Already shut down, ignore the request.
  }

  try {
    switch (pOp->Op()) {
      // IMFMediaSource methods:
//    case SourceOp::OP_START:
//      DoStart((StartOp*)pOp);
//      break;

//    case SourceOp::OP_STOP:
//      DoStop(pOp);
//      break;

      //    case SourceOp::OP_PAUSE:
      //      DoPause(pOp);
      //      break;

    case SourceOp::OP_SETRATE:
      DoSetRate(pOp);
      break;

      // Operations requested by the streams:
    case SourceOp::OP_REQUEST_DATA:
      OnStreamRequestSample(pOp);
      break;

    case SourceOp::OP_END_OF_STREAM:
      OnEndOfStream(pOp);
      break;

    default:
      ThrowException(E_UNEXPECTED);
    }
  }
  catch (Exception ^exc) {
    StreamingError(exc->HResult);
  }
#endif
  return S_OK;
}

//-------------------------------------------------------------------
// ValidateOperation
//
// Checks whether the source can perform the operation indicated
// by pOp at this time.
//
// If the source cannot perform the operation now, the method
// returns MF_E_NOTACCEPTING.
//
// NOTE:
// Implements the pure-virtual OpQueue::ValidateOperation method.
//-------------------------------------------------------------------

HRESULT Mp2tSource::ValidateOperation(SourceOp *pOp) {
#if 0
  if (m_spCurrentOp != nullptr) {
    ThrowException(MF_E_NOTACCEPTING);
  }
#endif
  return S_OK;
}

//-------------------------------------------------------------------
// DoStart
// Perform an async start operation (IMFMediaSource::Start)
//
// pOp: Contains the start parameters.
//
// Note: This sample currently does not implement seeking, and the
// Start() method fails if the caller requests a seek.
//-------------------------------------------------------------------

HRESULT Mp2tSource::DoStart(IMFPresentationDescriptor* pd, PROPVARIANT const*start_time) {
  auto hr = SelectStreams(pd, start_time);

  _state = STATE_STARTED;

  // Queue the "started" event. The event data is the start position.
  hr = _event_q->QueueEventParamVar(MESourceStarted, GUID_NULL, hr, start_time);
  return hr;
  // if (failed(hr))
 //    (void)m_spEventQueue->QueueEventParamVar(MESourceStarted, GUID_NULL, exc->HResult, nullptr);

   //CompleteAsyncOp(pOp);

#if 0
  //  assert(pOp->Op() == SourceOp::OP_START);

  BeginAsyncOp(pOp);

  try {
    ComPtr<IMFPresentationDescriptor> spPD;

    // Get the presentation descriptor from the SourceOp object.
    // This is the PD that the caller passed into the Start() method.
    // The PD has already been validated.
    ThrowIfError(pOp->GetPresentationDescriptor(&spPD));

    // Because this sample does not support seeking, the start
    // position must be 0 (from stopped) or "current position."

    // If the sample supported seeking, we would need to get the
    // start position from the PROPVARIANT data contained in pOp.

    // Select/deselect streams, based on what the caller set in the PD.
    // This method also sends the MENewStream/MEUpdatedStream events.
    SelectStreams(spPD.Get(), pOp->Data());

    m_state = STATE_STARTED;

    // Queue the "started" event. The event data is the start position.
    ThrowIfError(m_spEventQueue->QueueEventParamVar(
      MESourceStarted,
      GUID_NULL,
      S_OK,
      &pOp->Data()
      ));
  }
  catch (Exception ^exc) {
    // Failure. Send the error code to the application.

    // Note: It's possible that QueueEvent itself failed, in which case it
    // is likely to fail again. But there is no good way to recover in
    // that case.

    (void)m_spEventQueue->QueueEventParamVar(
      MESourceStarted, GUID_NULL, exc->HResult, nullptr);

    CompleteAsyncOp(pOp);

    throw;
  }

  CompleteAsyncOp(pOp);
#endif
}

//-------------------------------------------------------------------
// DoStop
// Perform an async stop operation (IMFMediaSource::Stop)
//-------------------------------------------------------------------

HRESULT Mp2tSource::DoStop() {
  Locker lock(this);

  // Stop the active streams.
  for (DWORD i = 0; i < _streams.GetCount(); i++) {
    if (_streams[i]->IsActive()) {
      _streams[i]->Stop();
    }
  }
  // Seek to the start of the file. If we restart after stopping,
  // we will start from the beginning of the file again.
  QWORD qwCurrentPosition = 0;
  auto hr = _byte_stream->Seek(msoBegin, 0, MFBYTESTREAM_SEEK_FLAG_CANCEL_PENDING_IO, &qwCurrentPosition);

  // Increment the counter that tracks "stale" read requests.
  ++m_cRestartCounter; // This counter is allowed to overflow.

  m_spSampleRequest = nullptr;

  _state = STATE_STOPPED;

  // Send the "stopped" event. This might include a failure code.
  hr = _event_q->QueueEventParamVar(MESourceStopped, GUID_NULL, hr, nullptr);
  return hr;
#if 0
  BeginAsyncOp(pOp);

  try {
    QWORD qwCurrentPosition = 0;

    // Stop the active streams.
    for (DWORD i = 0; i < m_streams.GetCount(); i++) {
      if (m_streams[i]->IsActive()) {
        m_streams[i]->Stop();
      }
    }

    // Seek to the start of the file. If we restart after stopping,
    // we will start from the beginning of the file again.
    ThrowIfError(m_spByteStream->Seek(
      msoBegin,
      0,
      MFBYTESTREAM_SEEK_FLAG_CANCEL_PENDING_IO,
      &qwCurrentPosition
      ));

    // Increment the counter that tracks "stale" read requests.
    ++m_cRestartCounter; // This counter is allowed to overflow.

    m_spSampleRequest.Reset();

    m_state = STATE_STOPPED;

    // Send the "stopped" event. This might include a failure code.
    (void)m_spEventQueue->QueueEventParamVar(MESourceStopped, GUID_NULL, S_OK, nullptr);
  }
  catch (Exception ^exc) {
    m_spSampleRequest.Reset();

    m_state = STATE_STOPPED;

    // Send the "stopped" event. This might include a failure code.
    (void)m_spEventQueue->QueueEventParamVar(MESourceStopped, GUID_NULL, exc->HResult, nullptr);

    CompleteAsyncOp(pOp);

    throw;
  }

  CompleteAsyncOp(pOp);
#endif
}

//-------------------------------------------------------------------
// DoPause
// Perform an async pause operation (IMFMediaSource::Pause)
//-------------------------------------------------------------------

HRESULT Mp2tSource::DoPause() {
  Locker lock(this);
  auto hr = S_OK;
  if (_state != STATE_STARTED)
    return _event_q->QueueEventParamVar(MESourcePaused, GUID_NULL, MF_E_INVALID_STATE_TRANSITION, nullptr);

  // Pause the active streams.
  for (DWORD i = 0; i < _streams.GetCount(); i++) {
    if (_streams[i]->IsActive()) {
      _streams[i]->Pause();
    }
  }
  _state = STATE_PAUSED;
  hr = _event_q->QueueEventParamVar(MESourcePaused, GUID_NULL, hr, nullptr);
  return hr;
#if 0
  BeginAsyncOp(pOp);

  try {
    // Pause is only allowed while running.
    if (m_state != STATE_STARTED) {
      ThrowException(MF_E_INVALID_STATE_TRANSITION);
    }

    // Pause the active streams.
    for (DWORD i = 0; i < m_streams.GetCount(); i++) {
      if (m_streams[i]->IsActive()) {
        m_streams[i]->Pause();
      }
    }

    m_state = STATE_PAUSED;

    // Send the "paused" event. This might include a failure code.
    (void)m_spEventQueue->QueueEventParamVar(MESourcePaused, GUID_NULL, S_OK, nullptr);
  }
  catch (Exception ^exc) {
    // Send the "paused" event. This might include a failure code.
    (void)m_spEventQueue->QueueEventParamVar(MESourcePaused, GUID_NULL, exc->HResult, nullptr);

    CompleteAsyncOp(pOp);

    throw;
  }

  CompleteAsyncOp(pOp);
#endif
}

//-------------------------------------------------------------------
// DoSetRate
// Perform an async set rate operation (IMFRateControl::SetRate)
//-------------------------------------------------------------------

HRESULT Mp2tSource::DoSetRate(float r) {
  Locker lock(this);
  _rate = r;
  // Set rate on active streams.
  for (DWORD i = 0; i < _streams.GetCount(); i++) {
    if (_streams[i]->IsActive()) {
      _streams[i]->SetRate(_rate);
    }
  }
  auto hr = _event_q->QueueEventParamVar(MESourceRateChanged, GUID_NULL, S_OK, nullptr);
  return hr;
#if 0
  SetRateOp *pSetRateOp = static_cast<SetRateOp*>(pOp);
  BeginAsyncOp(pOp);

  try {
    // Set rate on active streams.
    for (DWORD i = 0; i < m_streams.GetCount(); i++) {
      if (m_streams[i]->IsActive()) {
        m_streams[i]->SetRate(pSetRateOp->GetRate());
      }
    }

    m_flRate = pSetRateOp->GetRate();

    (void)m_spEventQueue->QueueEventParamVar(MESourceRateChanged, GUID_NULL, S_OK, nullptr);
  }
  catch (Exception ^exc) {
    // Send the "rate changted" event. This might include a failure code.
    (void)m_spEventQueue->QueueEventParamVar(MESourceRateChanged, GUID_NULL, exc->HResult, nullptr);

    CompleteAsyncOp(pOp);

    throw;
  }

  CompleteAsyncOp(pOp);
#endif
}

//-------------------------------------------------------------------
// StreamRequestSample
// Called by streams when they need more data.
//
// Note: This is an async operation. The stream requests more data
// by queueing an OP_REQUEST_DATA operation.
//-------------------------------------------------------------------

void Mp2tSource::OnStreamRequestSample(SourceOp *pOp) {
#if 0
  HRESULT hr = S_OK;

  BeginAsyncOp(pOp);

  // Ignore this request if we are already handling an earlier request.
  // (In that case m_pSampleRequest will be non-nullptr.)

  try {
    if (m_spSampleRequest == nullptr) {
      // Add the request counter as data to the operation.
      // This counter tracks whether a read request becomes "stale."

      PROPVARIANT var;
      var.vt = VT_UI4;
      var.ulVal = m_cRestartCounter;

      ThrowIfError(pOp->SetData(var));

      // Store this while the request is pending.
      m_spSampleRequest = pOp;

      // Try to parse data - this will invoke a read request if needed.
      ParseData();
    }
  }
  catch (Exception ^exc) {
    CompleteAsyncOp(pOp);
    throw;
  }

  CompleteAsyncOp(pOp);
#endif
}

//-------------------------------------------------------------------
// OnEndOfStream
// Called by each stream when it sends the last sample in the stream.
//
// Note: When the media source reaches the end of the MPEG-1 stream,
// it calls EndOfStream on each stream object. The streams might have
// data still in their queues. As each stream empties its queue, it
// notifies the source through an async OP_END_OF_STREAM operation.
//
// When every stream notifies the source, the source can send the
// "end-of-presentation" event.
//-------------------------------------------------------------------

void Mp2tSource::OnEndOfStream(SourceOp *pOp) {
#if 0
  BeginAsyncOp(pOp);

  try {
    --m_cPendingEOS;
    if (m_cPendingEOS == 0) {
      // No more streams. Send the end-of-presentation event.
      ThrowIfError(m_spEventQueue->QueueEventParamVar(MEEndOfPresentation, GUID_NULL, S_OK, nullptr));
    }
  }
  catch (Exception ^exc) {
    CompleteAsyncOp(pOp);

    throw;
  }

  CompleteAsyncOp(pOp);
#endif
}

//-------------------------------------------------------------------
// SelectStreams
// Called during START operations to select and deselect streams.
//-------------------------------------------------------------------

HRESULT Mp2tSource::SelectStreams(
  IMFPresentationDescriptor *pPD,   // Presentation descriptor.
  const PROPVARIANT *varStart) {// New start position.
  return S_OK;
#if 0
  BOOL fSelected = FALSE;
  BOOL fWasSelected = FALSE;
  DWORD   stream_id = 0;

  CMPEG1Stream *wpStream = nullptr; // Not add-ref'd

  // Reset the pending EOS count.
  m_cPendingEOS = 0;

  // Loop throught the stream descriptors to find which streams are active.
  for (DWORD i = 0; i < m_streams.GetCount(); i++) {
    ComPtr<IMFStreamDescriptor> spSD;

    ThrowIfError(pPD->GetStreamDescriptorByIndex(i, &fSelected, &spSD));
    ThrowIfError(spSD->GetStreamIdentifier(&stream_id));

    wpStream = m_streams.Find((BYTE)stream_id);
    if (wpStream == nullptr) {
      ThrowException(E_INVALIDARG);
    }

    // Was the stream active already?
    fWasSelected = wpStream->IsActive();

    // Activate or deactivate the stream.
    wpStream->Activate(!!fSelected);

    if (fSelected) {
      m_cPendingEOS++;

      // If the stream was previously selected, send an "updated stream"
      // event. Otherwise, send a "new stream" event.
      MediaEventType met = fWasSelected ? MEUpdatedStream : MENewStream;

      ThrowIfError(m_spEventQueue->QueueEventParamUnk(met, GUID_NULL, S_OK, static_cast<IMFMediaStream*>(wpStream)));

      // Start the stream. The stream will send the appropriate event.
      wpStream->Start(varStart);
    }
  }
#endif
}

//-------------------------------------------------------------------
// RequestData
// Request the next batch of data.
//
// cbRequest: Amount of data to read, in bytes.
//-------------------------------------------------------------------
HRESULT Mp2tSource::BeginRequestData(BYTE *buf, ULONG blen, IMFAsyncCallback*cb, IUnknown*stat) {
  Locker lock(this);
  //  auto hr = buf->Lock(&b, &len, nullptr);
  auto hr = _byte_stream->BeginRead(buf, blen, cb, stat);
  return hr;
}
HRESULT Mp2tSource::EndRequestData(IMFAsyncResult* result, ULONG *readed) {
  Locker lock(this);
  auto hr = _byte_stream->EndRead(result, readed);
  return hr;
}

void Mp2tSource::RequestData(DWORD cbRequest) {
  auto buf = CreateBuffer(cbRequest);
  ComPtr<Mp2tSource> me(this);
  auto bytes = _byte_stream;
  auto cb = CreateAsyncCallback([buf, me, bytes](IMFAsyncResult*result) ->HRESULT {
    ULONG readed = 0;
    auto hr = bytes->EndRead(result, &readed);
    if (ok(hr)) hr = buf->SetCurrentLength(readed);
    if (ok(hr)) me->OnByteStreamRead(buf.Get());
    buf->Unlock();
    return hr;
  });
  BYTE* b = nullptr;
  ULONG len = 0;
  auto hr = buf->Lock(&b, &len, nullptr);
  if (ok(hr)) hr = _byte_stream->BeginRead(b, len, cb.Get(), nullptr);
  //  auto hr = _byte_stream->BeginRead(m_ReadBuffer->DataPtr + m_ReadBuffer->DataSize, cbRequest, cb.Get(), m_spSampleRequest.Get());
#if 0
  // Reserve a sufficient read buffer.
  m_ReadBuffer->Reserve(cbRequest);

  // Submit the async read request.
  // When it completes, our OnByteStreamRead method will be invoked.

  ThrowIfError(m_spByteStream->BeginRead(
    m_ReadBuffer->DataPtr + m_ReadBuffer->DataSize,
    cbRequest,
    &m_OnByteStreamRead,
    m_spSampleRequest.Get()
    ));
#endif
}

//-------------------------------------------------------------------
// ParseData
// Parses the next batch of data.
//-------------------------------------------------------------------

void Mp2tSource::ParseData() {
#if 0
  HRESULT hr = S_OK;

  DWORD cbNextRequest = 0;
  bool  fNeedMoreData = false;

  // Keep processing data until
  // (a) All streams have enough samples, or
  // (b) The parser needs more data in the buffer.

  while (StreamsNeedData()) {
    DWORD cbAte = 0;    // How much data we consumed from the read buffer.

    // Check if we got the first system header.
    if (m_header == nullptr && m_parser->HasSystemHeader) {
      m_header = m_parser->GetSystemHeader();
    }

    if (m_parser->IsEndOfStream) {
      // The parser reached the end of the MPEG-1 stream. Notify the streams.
      EndOfMPEGStream();
    }
    else if (m_parser->HasPacket) {
      // The parser reached the start of a new packet.
      fNeedMoreData = !ReadPayload(&cbAte, &cbNextRequest);
    }
    else {
      // Parse more data.
      fNeedMoreData = !m_parser->ParseBytes(m_ReadBuffer->DataPtr, m_ReadBuffer->DataSize, &cbAte);
    }

    // Advance the start of the read buffer by the amount consumed.
    m_ReadBuffer->MoveStart(cbAte);

    // If we need more data, start an async read operation.
    if (fNeedMoreData) {
      RequestData(max(READ_SIZE, cbNextRequest));

      // Break from the loop because we need to wait for the async read to complete.
      break;
    }
  }

  // Flag our state. If a stream requests more data while we are waiting for an async
  // read to complete, we can ignore the stream's request, because the request will be
  // dispatched as soon as we get more data.

  if (!fNeedMoreData) {
    m_spSampleRequest.Reset();
  }
#endif
}

//-------------------------------------------------------------------
// ReadPayload
// Read the next MPEG-1 payload.
//
// When this method is called:
// - The read position has reached the beginning of a payload.
// - We have the packet header, but not necessarily the entire payload.
//-------------------------------------------------------------------

bool Mp2tSource::ReadPayload(DWORD *pcbAte, DWORD *pcbNextRequest) {
  return false;
#if 0
  assert(m_parser != nullptr);
  assert(m_parser->HasPacket);
  bool fResult = true;
  DWORD cbPayloadRead = 0;
  DWORD cbPayloadUnread = 0;

  // At this point, the read buffer might be larger or smaller than the payload.
  // Calculate which portion of the payload has been read.
  if (m_parser->PayloadSize > m_ReadBuffer->DataSize) {
    cbPayloadUnread = m_parser->PayloadSize - m_ReadBuffer->DataSize;
  }

  cbPayloadRead = m_parser->PayloadSize - cbPayloadUnread;

  // Do we need to deliver this payload?
  if (!IsStreamActive(m_parser->PacketHeader)) {
    QWORD qwCurrentPosition = 0;

    // Skip this payload. Seek past the unread portion of the payload.
    ThrowIfError(m_spByteStream->Seek(
      msoCurrent,
      cbPayloadUnread,
      MFBYTESTREAM_SEEK_FLAG_CANCEL_PENDING_IO,
      &qwCurrentPosition
      ));

    // Advance the data buffer to the end of payload, or the portion
    // that has been read.

    *pcbAte = cbPayloadRead;

    // Tell the parser that we are done with this packet.
    m_parser->ClearPacket();
  }
  else if (cbPayloadUnread > 0) {
    // Some portion of this payload has not been read. Schedule a read.
    *pcbNextRequest = cbPayloadUnread;

    *pcbAte = 0;

    fResult = false; // Need more data.
  }
  else {
    // The entire payload is in the data buffer. Deliver the packet.
    DeliverPayload();

    *pcbAte = cbPayloadRead;

    // Tell the parser that we are done with this packet.
    m_parser->ClearPacket();
  }

  return fResult;
#endif
}

//-------------------------------------------------------------------
// EndOfMPEGStream:
// Called when the parser reaches the end of the MPEG1 stream.
//-------------------------------------------------------------------

void Mp2tSource::EndOfMPEGStream() {
  // Notify the streams. The streams might have pending samples.
  // When each stream delivers the last sample, it will send the
  // end-of-stream event to the pipeline and then notify the
  // source.

  // When every stream is done, the source sends the end-of-
  // presentation event.

  for (DWORD i = 0; i < _streams.GetCount(); i++) {
    if (_streams[i]->IsActive()) {
      _streams[i]->EndOfStream();
    }
  }
}

//-------------------------------------------------------------------
// StreamsNeedData:
// Returns TRUE if any streams need more data.
//-------------------------------------------------------------------

bool Mp2tSource::StreamsNeedData() const {
  bool fNeedData = false;

  switch (_state) {
  case STATE_OPENING:
    // While opening, we always need data (until we get enough
    // to complete the open operation).
    return true;

  case STATE_SHUTDOWN:
    // While shut down, we never need data.
    return false;

  default:
    // If none of the above, ask the streams.
    for (DWORD i = 0; i < _streams.GetCount(); i++) {
      if (_streams[i]->NeedsData()) {
        fNeedData = true;
        break;
      }
    }
    return fNeedData;
  }
}

//-------------------------------------------------------------------
// DeliverPayload:
// Delivers an MPEG-1 payload.
//-------------------------------------------------------------------

void Mp2tSource::DeliverPayload() {
#if 0
  // When this method is called, the read buffer contains a complete
  // payload, and the payload belongs to a stream whose type we support.

  assert(m_parser->HasPacket);

  MPEG1PacketHeader packetHdr;
  CMPEG1Stream *wpStream = nullptr;   // not AddRef'd

  ComPtr<IMFMediaBuffer> spBuffer;
  ComPtr<IMFSample> spSample;
  BYTE *pData = nullptr;              // Pointer to the IMFMediaBuffer data.

  packetHdr = m_parser->PacketHeader;

  if (packetHdr.cbPayload > m_ReadBuffer->DataSize) {
    assert(FALSE);
    ThrowException(E_UNEXPECTED);
  }

  // If we are still opening the file, then we might need to create this stream.
  if (m_state == STATE_OPENING) {
    CreateStream(packetHdr);
  }

  wpStream = m_streams.Find(packetHdr.stream_id);
  assert(wpStream != nullptr);

  // Create a media buffer for the payload.
  ThrowIfError(MFCreateMemoryBuffer(packetHdr.cbPayload, &spBuffer));

  ThrowIfError(spBuffer->Lock(&pData, nullptr, nullptr));

  CopyMemory(pData, m_ReadBuffer->DataPtr, packetHdr.cbPayload);

  ThrowIfError(spBuffer->Unlock());

  ThrowIfError(spBuffer->SetCurrentLength(packetHdr.cbPayload));

  ThrowIfError(MFCreateSample(&spSample));
  ThrowIfError(spSample->AddBuffer(spBuffer.Get()));

  if (packetHdr.bHasPTS) {
    LONGLONG hnsStart = packetHdr.PTS * 10000ll / 90ll;

    ThrowIfError(spSample->SetSampleTime(hnsStart));
  }

  // Deliver the payload to the stream.
  wpStream->DeliverPayload(spSample.Get());

  // If the open operation is still pending, check if we're done.
  if (m_state == STATE_OPENING) {
    InitPresentationDescriptor();
  }
#endif
}

//-------------------------------------------------------------------
// CreateStream:
// Creates a media stream, based on a packet header.
//-------------------------------------------------------------------

void Mp2tSource::CreateStream(const Mp2tHeader &packetHdr) {
#if 0
  // We validate the stream type before calling this method.
  assert(IsStreamTypeSupported(packetHdr.type));

  // First see if the stream already exists.
  if (m_streams.Find(packetHdr.stream_id) != nullptr) {
    // The stream already exists. Nothing to do.
    return;
  }

  DWORD cbAte = 0;
  BYTE *pPayload = nullptr;

  ComPtr<IMFMediaType> spType;
  ComPtr<IMFStreamDescriptor> spSD;
  ComPtr<CMPEG1Stream> spStream;
  ComPtr<IMFMediaTypeHandler> spHandler;

  MPEG1VideoSeqHeader videoSeqHdr;
  MPEG1AudioFrameHeader audioFrameHeader;

  // Get a pointer to the start of the payload.
  pPayload = m_ReadBuffer->DataPtr;

  // Create a media type, based on the packet type (audio/video)
  switch (packetHdr.type) {
  case StreamType_Video:
    // Video: Read the sequence header and use it to create a media type.
    cbAte = ReadVideoSequenceHeader(pPayload, packetHdr.cbPayload, videoSeqHdr);
    spType = CreateVideoMediaType(videoSeqHdr);
    break;

  case StreamType_Audio:
    // Audio: Read the frame header and use it to create a media type.
    cbAte = ReadAudioFrameHeader(pPayload, packetHdr.cbPayload, audioFrameHeader);
    spType = CreateAudioMediaType(audioFrameHeader);
    break;

  default:
    assert(false); // If this case occurs, then IsStreamTypeSupported() is wrong.
    ThrowException(E_UNEXPECTED);
  }

  // Create the stream descriptor from the media type.
  ThrowIfError(MFCreateStreamDescriptor(packetHdr.stream_id, 1, spType.GetAddressOf(), &spSD));
  // Set the default media type on the stream handler.
  ThrowIfError(spSD->GetMediaTypeHandler(&spHandler));
  ThrowIfError(spHandler->SetCurrentMediaType(spType.Get()));

  // Create the new stream.
//  spStream.Attach(new (std::nothrow) CMPEG1Stream(this, spSD.Get()));
  spStream = Make<CMPEG1Stream>(this, spSD.Get());
  if (spStream == nullptr) {
    throw ref new OutOfMemoryException();
  }
  spStream->Initialize();

  // Add the stream to the array.
  ThrowIfError(m_streams.AddStream(packetHdr.stream_id, spStream.Get()));
#endif
}

//-------------------------------------------------------------------
// ValidatePresentationDescriptor:
// Validates the presentation descriptor that the caller specifies
// in IMFMediaSource::Start().
//
// Note: This method performs a basic sanity check on the PD. It is
// not intended to be a thorough validation.
//-------------------------------------------------------------------

HRESULT Mp2tSource::ValidatePresentationDescriptor(IMFPresentationDescriptor *pPD) {
#if 0
  try {
    BOOL fSelected = FALSE;
    DWORD cStreams = 0;

    if (m_header == nullptr) {
      ThrowException(E_UNEXPECTED);
    }

    // The caller's PD must have the same number of streams as ours.
    ThrowIfError(pPD->GetStreamDescriptorCount(&cStreams));

    if (cStreams != m_header->Get()->cStreams) {
      ThrowException(E_INVALIDARG);
    }

    // The caller must select at least one stream.
    for (DWORD i = 0; i < cStreams; i++) {
      ComPtr<IMFStreamDescriptor> spSD;
      ThrowIfError(pPD->GetStreamDescriptorByIndex(i, &fSelected, &spSD));
      if (fSelected) {
        break;
      }
    }

    if (!fSelected) {
      throw ref new InvalidArgumentException();
    }
  }
  catch (Exception ^exc) {
    return exc->HResult;
  }
#endif
  return S_OK;
}

//-------------------------------------------------------------------
// StreamingError:
// Handles an error that occurs duing an asynchronous operation.
//
// hr: Error code of the operation that failed.
//-------------------------------------------------------------------

void Mp2tSource::StreamingError(HRESULT hr) {
  if (_state == STATE_OPENING) {
    // An error happened during BeginOpen.
    // Invoke the callback with the status code.

    CompleteOpen(hr);
  }
  else if (_state != STATE_SHUTDOWN) {
    // An error occurred during streaming. Send the MEError event
    // to notify the pipeline.

    QueueEvent(MEError, GUID_NULL, hr, nullptr);
  }
}

bool Mp2tSource::IsRateSupported(float flRate, float *pflAdjustedRate) {
  if (flRate < 0.00001f && flRate > -0.00001f) {
    *pflAdjustedRate = 0.0f;
    return true;
  }
  else if (flRate < 1.0001f && flRate > 0.9999f) {
    *pflAdjustedRate = 1.0f;
    return true;
  }
  return false;
}

/* SourceOp class */

//-------------------------------------------------------------------
// CreateOp
// Static method to create a SourceOp instance.
//
// op: Specifies the async operation.
// ppOp: Receives a pointer to the SourceOp object.
//-------------------------------------------------------------------

HRESULT SourceOp::CreateOp(SourceOp::Operation op, SourceOp **ppOp) {
  if (ppOp == nullptr) {
    return E_POINTER;
  }

  SourceOp *pOp = new (std::nothrow) SourceOp(op);
  if (pOp == nullptr) {
    return E_OUTOFMEMORY;
  }
  *ppOp = pOp;

  return S_OK;
}

//-------------------------------------------------------------------
// CreateStartOp:
// Static method to create a SourceOp instance for the Start()
// operation.
//
// pPD: Presentation descriptor from the caller.
// ppOp: Receives a pointer to the SourceOp object.
//-------------------------------------------------------------------
#if 0
HRESULT SourceOp::CreateStartOp(IMFPresentationDescriptor *pPD, SourceOp **ppOp) {
  if (ppOp == nullptr) {
    return E_POINTER;
  }

  SourceOp *pOp = new (std::nothrow) StartOp(pPD);
  if (pOp == nullptr) {
    return E_OUTOFMEMORY;
  }

  *ppOp = pOp;
  return S_OK;
}
#endif
//-------------------------------------------------------------------
// CreateSetRateOp:
// Static method to create a SourceOp instance for the SetRate()
// operation.
//
// fThin: TRUE - thinning is on, FALSE otherwise
// flRate: New rate
// ppOp: Receives a pointer to the SourceOp object.
//-------------------------------------------------------------------

HRESULT SourceOp::CreateSetRateOp(BOOL fThin, float flRate, SourceOp **ppOp) {
  if (ppOp == nullptr) {
    return E_POINTER;
  }

  SourceOp *pOp = new (std::nothrow) SetRateOp(fThin, flRate);
  if (pOp == nullptr) {
    return E_OUTOFMEMORY;
  }

  *ppOp = pOp;
  return S_OK;
}

ULONG SourceOp::AddRef() {
  return _InterlockedIncrement(&m_cRef);
}

ULONG SourceOp::Release() {
  LONG cRef = _InterlockedDecrement(&m_cRef);
  if (cRef == 0) {
    delete this;
  }
  return cRef;
}

HRESULT SourceOp::QueryInterface(REFIID riid, void **ppv) {
  if (ppv == nullptr) {
    return E_POINTER;
  }

  HRESULT hr = E_NOINTERFACE;
  if (riid == IID_IUnknown) {
    (*ppv) = static_cast<IUnknown *>(this);
    AddRef();
    hr = S_OK;
  }

  return hr;
}

SourceOp::SourceOp(Operation op) : m_cRef(1), m_op(op) {
  ZeroMemory(&m_data, sizeof(m_data));
}

SourceOp::~SourceOp() {
  PropVariantClear(&m_data);
}

HRESULT SourceOp::SetData(const PROPVARIANT &var) {
  return PropVariantCopy(&m_data, &var);
}
#if 0
StartOp::StartOp(IMFPresentationDescriptor *pPD) : SourceOp(SourceOp::OP_START), m_spPD(pPD) {}

StartOp::~StartOp() {}

HRESULT StartOp::GetPresentationDescriptor(IMFPresentationDescriptor **ppPD) {
  if (ppPD == nullptr) {
    return E_POINTER;
  }
  if (m_spPD == nullptr) {
    return MF_E_INVALIDREQUEST;
  }
  *ppPD = m_spPD.Get();
  (*ppPD)->AddRef();
  return S_OK;
}
#endif
SetRateOp::SetRateOp(BOOL fThin, float flRate)
  : SourceOp(SourceOp::OP_SETRATE)
  , m_fThin(fThin)
  , m_flRate(flRate) {}

SetRateOp::~SetRateOp() {}

/*  Static functions */

//-------------------------------------------------------------------
// CreateVideoMediaType:
// Create a media type from an MPEG-1 video sequence header.
//-------------------------------------------------------------------

ComPtr<IMFMediaType> CreateVideoMediaType(const Mp2tHeader &videoSeqHdr) {
  return ComPtr<IMFMediaType>();
#if 0
  ComPtr<IMFMediaType> spType;

  ThrowIfError(MFCreateMediaType(&spType));

  ThrowIfError(spType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));

  ThrowIfError(spType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_MPG1));

  // Format details.

  // Frame size
  ThrowIfError(MFSetAttributeSize(
    spType.Get(),
    MF_MT_FRAME_SIZE,
    videoSeqHdr.width,
    videoSeqHdr.height
    ));

  // Frame rate
  ThrowIfError(MFSetAttributeRatio(
    spType.Get(),
    MF_MT_FRAME_RATE,
    videoSeqHdr.frameRate.Numerator,
    videoSeqHdr.frameRate.Denominator
    ));

  // Pixel aspect ratio
  ThrowIfError(MFSetAttributeRatio(
    spType.Get(),
    MF_MT_PIXEL_ASPECT_RATIO,
    videoSeqHdr.pixelAspectRatio.Numerator,
    videoSeqHdr.pixelAspectRatio.Denominator
    ));

  // Average bit rate
  ThrowIfError(spType->SetUINT32(MF_MT_AVG_BITRATE, videoSeqHdr.bitRate));

  // Interlacing (progressive frames)
  ThrowIfError(spType->SetUINT32(MF_MT_INTERLACE_MODE, (UINT32)MFVideoInterlace_Progressive));

  // Sequence header.
  ThrowIfError(spType->SetBlob(
    MF_MT_MPEG_SEQUENCE_HEADER,
    videoSeqHdr.header,             // Byte array
    videoSeqHdr.cbHeader            // Size
    ));

  return spType;
#endif
}

//-------------------------------------------------------------------
// CreateAudioMediaType:
// Create a media type from an MPEG-1 audio frame header.
//
// Note: This function fills in an MPEG1WAVEFORMAT structure and then
// converts the structure to a Media Foundation media type
// (IMFMediaType). This is somewhat roundabout but it guarantees
// that the type can be converted back to an MPEG1WAVEFORMAT by the
// decoder if need be.
//
// The WAVEFORMATEX portion of the MPEG1WAVEFORMAT structure is
// converted into attributes on the IMFMediaType object. The rest of
// the struct is stored in the MF_MT_USER_DATA attribute.
//-------------------------------------------------------------------

ComPtr<IMFMediaType> CreateAudioMediaType(const Mp2tHeader &audioHeader) {
  return ComPtr<IMFMediaType>();
#if 0
  ComPtr<IMFMediaType> spType;

  MPEG1WAVEFORMAT format;
  ZeroMemory(&format, sizeof(format));

  format.wfx.wFormatTag = WAVE_FORMAT_MPEG;
  format.wfx.nChannels = audioHeader.nChannels;
  format.wfx.nSamplesPerSec = audioHeader.dwSamplesPerSec;
  if (audioHeader.dwBitRate > 0) {
    format.wfx.nAvgBytesPerSec = (audioHeader.dwBitRate * 1000) / 8;
  }
  format.wfx.nBlockAlign = audioHeader.nBlockAlign;
  format.wfx.wBitsPerSample = 0; // Not used.
  format.wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX);

  // MPEG-1 audio layer.
  switch (audioHeader.layer) {
  case MPEG1_Audio_Layer1:
    format.fwHeadLayer = ACM_MPEG_LAYER1;
    break;

  case MPEG1_Audio_Layer2:
    format.fwHeadLayer = ACM_MPEG_LAYER2;
    break;

  case MPEG1_Audio_Layer3:
    format.fwHeadLayer = ACM_MPEG_LAYER3;
    break;
  };

  format.dwHeadBitrate = audioHeader.dwBitRate * 1000;

  // Mode
  switch (audioHeader.mode) {
  case MPEG1_Audio_Stereo:
    format.fwHeadMode = ACM_MPEG_STEREO;
    break;

  case MPEG1_Audio_JointStereo:
    format.fwHeadMode = ACM_MPEG_JOINTSTEREO;
    break;

  case MPEG1_Audio_DualChannel:
    format.fwHeadMode = ACM_MPEG_DUALCHANNEL;
    break;

  case MPEG1_Audio_SingleChannel:
    format.fwHeadMode = ACM_MPEG_SINGLECHANNEL;
    break;
  };

  if (audioHeader.mode == ACM_MPEG_JOINTSTEREO) {
    // Convert the 'mode_extension' field to the correct MPEG1WAVEFORMAT value.
    if (audioHeader.modeExtension <= 0x03) {
      format.fwHeadModeExt = 0x01 << audioHeader.modeExtension;
    }
  }

  // Convert the 'emphasis' field to the correct MPEG1WAVEFORMAT value.
  if (audioHeader.emphasis <= 0x03) {
    format.wHeadEmphasis = audioHeader.emphasis + 1;
  }

  // The flags translate directly.
  format.fwHeadFlags = audioHeader.wFlags;
  // Add the "MPEG-1" flag, although it's somewhat redundant.
  format.fwHeadFlags |= ACM_MPEG_ID_MPEG1;

  // Use the structure to initialize the Media Foundation media type.
  ThrowIfError(MFCreateMediaType(&spType));
  ThrowIfError(MFInitMediaTypeFromWaveFormatEx(spType.Get(), (const WAVEFORMATEX*)&format, sizeof(format)));

  return spType;
#endif
}

// Get the major media type from a stream descriptor.
void GetStreamMajorType(IMFStreamDescriptor *pSD, GUID *pguidMajorType) {
#if 0
  if (pSD == nullptr || pguidMajorType == nullptr) {
    throw ref new InvalidArgumentException();
  }

  ComPtr<IMFMediaTypeHandler> spHandler;

  ThrowIfError(pSD->GetMediaTypeHandler(&spHandler));
  ThrowIfError(spHandler->GetMajorType(pguidMajorType));
#endif
}

#pragma warning( pop )