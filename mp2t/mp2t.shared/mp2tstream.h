#pragma once

class Mp2tSource;

// The media stream object., IMFMediaEventGenerator
class Mp2tStream : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IMFMediaStream> {
public:

  Mp2tStream(Mp2tSource *pSource, IMFStreamDescriptor *pSD);
  ~Mp2tStream();
  void Initialize();

  // IMFMediaEventGenerator
  STDMETHODIMP BeginGetEvent(IMFAsyncCallback *pCallback, IUnknown *punkState);
  STDMETHODIMP EndGetEvent(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent);
  STDMETHODIMP GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent);
  STDMETHODIMP QueueEvent(MediaEventType met, REFGUID guidExtendedType, HRESULT hrStatus, const PROPVARIANT *pvValue);

  // IMFMediaStream
  STDMETHODIMP GetMediaSource(IMFMediaSource **ppMediaSource);
  STDMETHODIMP GetStreamDescriptor(IMFStreamDescriptor **ppStreamDescriptor);
  STDMETHODIMP RequestSample(IUnknown *pToken);

  // Other methods (called by source)
  void     Activate(bool fActive);
  void     Start(const PROPVARIANT &varStart);
  void     Pause();
  void     Stop();
  void     SetRate(float flRate);
  void     EndOfStream();
  void     Shutdown();

  bool      IsActive() const { return m_fActive; }
  bool      NeedsData();

  void   DeliverPayload(IMFSample *pSample);

  // Callbacks
//  HRESULT     OnDispatchSamples(IMFAsyncResult *pResult);

private:

  HRESULT CheckShutdown() const {
    return (m_state == STATE_SHUTDOWN ? MF_E_SHUTDOWN : S_OK);
  }
  void DispatchSamples() throw();

private:
  long                m_cRef;                 // reference count

  ComPtr<Mp2tSource> m_spSource;            // Parent media source
  ComPtr<IMFStreamDescriptor> m_spStreamDescriptor;
  ComPtr<IMFMediaEventQueue> m_spEventQueue;  // Event generator helper

  SourceState         m_state;                // Current state (running, stopped, paused)
  bool                m_fActive;              // Is the stream active?
  bool                m_fEOS;                 // Did the source reach the end of the stream?

  SampleList          m_Samples;              // Samples waiting to be delivered.
  TokenList           m_Requests;             // Sample requests, waiting to be dispatched.

  float               m_flRate;
};
