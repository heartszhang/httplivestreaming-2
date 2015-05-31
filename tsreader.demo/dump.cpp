#include "pch.h"
#include <strsafe.h>
//#include <propvarutil.h>

void dump( const wchar_t*fmt, ... ) {
  wchar_t buf[ 4096 ];
  va_list va;
  va_start( va, fmt );
  auto r = StringCchVPrintfW( buf, _countof( buf ), fmt, va );  // ignore return value
  va_end( va );
  if ( ok( r ) || r == STRSAFE_E_INSUFFICIENT_BUFFER )
    OutputDebugStringW( buf );
}
static struct mediasource_char {
  DWORD id;
  const wchar_t* name;
}mediasource_chars[] {{MFMEDIASOURCE_IS_LIVE, L"MFMEDIASOURCE_IS_LIVE"}
, { MFMEDIASOURCE_CAN_SEEK , L"MFMEDIASOURCE_CAN_SEEK"}
, { MFMEDIASOURCE_CAN_PAUSE , L"MFMEDIASOURCE_CAN_PAUSE"}
, { MFMEDIASOURCE_HAS_SLOW_SEEK, L"MFMEDIASOURCE_HAS_SLOW_SEEK" }
, { MFMEDIASOURCE_HAS_MULTIPLE_PRESENTATIONS, L"MFMEDIASOURCE_HAS_MULTIPLE_PRESENTATIONS" }
, { MFMEDIASOURCE_CAN_SKIPFORWARD, L"MFMEDIASOURCE_CAN_SKIPFORWARD" }
, { MFMEDIASOURCE_CAN_SKIPBACKWARD, L"MFMEDIASOURCE_CAN_SKIPBACKWARD" }
, { MFMEDIASOURCE_DOES_NOT_USE_NETWORK, L"MFMEDIASOURCE_DOES_NOT_USE_NETWORK" }};
static struct event_name {
  __MIDL___MIDL_itf_mfobjects_0000_0012_0001  id;
  const wchar_t* name;
}event_names[] = { { MEUnknown, L"MEUnknown" }
, { MEError, L"MEError" }
, { MEExtendedType, L"MEExtendedType" }
, { MENonFatalError, L"MENonFatalError" }
, { MEGenericV1Anchor, L"MEGenericV1Anchor" }
, { MESessionUnknown, L"MESessionUnknown" }
, { MESessionTopologySet, L"MESessionTopologySet" }
, { MESessionTopologiesCleared, L"MESessionTopologiesCleared" }
, { MESessionStarted, L"MESessionStarted" }
, { MESessionPaused, L"MESessionPaused" }
, { MESessionStopped, L"MESessionStopped" },
{MESessionClosed, L"MESessionClosed"},
{ MESessionEnded, L"MESessionEnded" },
{ MESessionRateChanged, L"MESessionRateChanged" },
{ MESessionScrubSampleComplete, L"MESessionScrubSampleComplete" },
{ MESessionCapabilitiesChanged, L"MESessionCapabilitiesChanged" },
{ MESessionTopologyStatus, L"MESessionTopologyStatus" },
{ MESessionNotifyPresentationTime, L"MESessionNotifyPresentationTime" },
{ MENewPresentation, L"MENewPresentation" },
{ MELicenseAcquisitionStart, L"MELicenseAcquisitionStart" },
{ MELicenseAcquisitionCompleted, L"MELicenseAcquisitionCompleted" },
{ MEIndividualizationStart, L"MEIndividualizationStart" },
{ MEIndividualizationCompleted, L"MEIndividualizationCompleted" },
{ MEEnablerProgress, L"MEEnablerProgress" },
{ MEEnablerCompleted, L"MEEnablerCompleted" },
{ MEPolicyError, L"MEPolicyError" },
{ MEPolicyReport, L"MEPolicyReport" },
{ MEBufferingStarted, L"MEBufferingStarted" },
{ MEBufferingStopped, L"MEBufferingStopped" },
{ MEConnectStart, L"MEConnectStart" },
{ MEConnectEnd, L"MEConnectEnd" },
{ MEReconnectStart, L"MEReconnectStart" },
{ MEReconnectEnd, L"MEReconnectEnd" },
{ MERendererEvent, L"MERendererEvent" },
{ MESessionStreamSinkFormatChanged, L"MESessionStreamSinkFormatChanged" },
{ MESourceUnknown, L"MESourceUnknown" },
{ MESourceStarted, L"MESourceStarted" },
{ MEStreamStarted, L"MEStreamStarted" },
{ MESourceSeeked, L"MESourceSeeked" },
{ MEStreamSeeked, L"MEStreamSeeked" },
{ MENewStream, L"MENewStream" },
{ MEUpdatedStream, L"MEUpdatedStream" },
{ MESourceStopped, L"MESourceStopped" },
{ MEStreamStopped, L"MEStreamStopped" },
{ MESourcePaused, L"MESourcePaused" },
{ MEStreamPaused, L"MEStreamPaused" },
{ MEEndOfPresentation, L"MEEndOfPresentation" },
{ MEEndOfStream, L"MEEndOfStream" },
{ MEMediaSample, L"MEMediaSample" },
{ MEStreamTick, L"MEStreamTick" },
{ MEStreamThinMode, L"MEStreamThinMode" },
{ MEStreamFormatChanged, L"MEStreamFormatChanged" },
{ MESourceRateChanged, L"MESourceRateChanged" },
{ MEEndOfPresentationSegment, L"MEEndOfPresentationSegment" },
{ MESourceCharacteristicsChanged, L"MESourceCharacteristicsChanged" },
{ MESourceRateChangeRequested, L"MESourceRateChangeRequested" },
{ MESourceMetadataChanged, L"MESourceMetadataChanged" },
{ MESequencerSourceTopologyUpdated, L"MESequencerSourceTopologyUpdated" },
{ MESinkUnknown, L"MESinkUnknown" },
{ MEStreamSinkStarted, L"MEStreamSinkStarted" },
{ MEStreamSinkStopped, L"MEStreamSinkStopped" },
{ MEStreamSinkPaused, L"MEStreamSinkPaused" },
{ MEStreamSinkRateChanged, L"MEStreamSinkRateChanged" },
{ MEStreamSinkRequestSample, L"MEStreamSinkRequestSample" },
{ MEStreamSinkMarker, L"MEStreamSinkMarker" },
{ MEStreamSinkPrerolled, L"MEStreamSinkPrerolled" },
{ MEStreamSinkScrubSampleComplete, L"MEStreamSinkScrubSampleComplete" },
{ MEStreamSinkFormatChanged, L"MEStreamSinkFormatChanged" },
{ MEStreamSinkDeviceChanged, L"MEStreamSinkDeviceChanged" },
{ MEQualityNotify, L"MEQualityNotify" },
{ MESinkInvalidated, L"MESinkInvalidated" },
{ MEAudioSessionNameChanged, L"MEAudioSessionNameChanged" },
{ MEAudioSessionVolumeChanged, L"MEAudioSessionVolumeChanged" },
{ MEAudioSessionDeviceRemoved, L"MEAudioSessionDeviceRemoved" },
{ MEAudioSessionServerShutdown, L"MEAudioSessionServerShutdown" },
{ MEAudioSessionGroupingParamChanged, L"MEAudioSessionGroupingParamChanged" },
{ MEAudioSessionIconChanged, L"MEAudioSessionIconChanged" },
{ MEAudioSessionFormatChanged, L"MEAudioSessionFormatChanged" },
{ MEAudioSessionDisconnected, L"MEAudioSessionDisconnected" },
{ MEAudioSessionExclusiveModeOverride, L"MEAudioSessionExclusiveModeOverride" },
{ MECaptureAudioSessionVolumeChanged, L"MECaptureAudioSessionVolumeChanged" },
{ MECaptureAudioSessionDeviceRemoved, L"MECaptureAudioSessionDeviceRemoved" },
{ MECaptureAudioSessionFormatChanged, L"MECaptureAudioSessionFormatChanged" },
{ MECaptureAudioSessionDisconnected, L"MECaptureAudioSessionDisconnected" },
{ MECaptureAudioSessionExclusiveModeOverride, L"MECaptureAudioSessionExclusiveModeOverride" },
{ MECaptureAudioSessionServerShutdown, L"MECaptureAudioSessionServerShutdown" },
{ METrustUnknown, L"METrustUnknown" },
{ MEPolicyChanged, L"MEPolicyChanged" },
{ MEContentProtectionMessage, L"MEContentProtectionMessage" },
{ MEPolicySet, L"MEPolicySet" },
{ MEWMDRMLicenseBackupCompleted, L"MEWMDRMLicenseBackupCompleted" },
{ MEWMDRMLicenseBackupProgress, L"MEWMDRMLicenseBackupProgress" },
{ MEWMDRMLicenseRestoreCompleted, L"MEWMDRMLicenseRestoreCompleted" },
{ MEWMDRMLicenseRestoreProgress, L"MEWMDRMLicenseRestoreProgress" },
{ MEWMDRMLicenseAcquisitionCompleted, L"MEWMDRMLicenseAcquisitionCompleted" },
{ MEWMDRMIndividualizationCompleted, L"MEWMDRMIndividualizationCompleted" },
{ MEWMDRMIndividualizationProgress, L"MEWMDRMIndividualizationProgress" },
{ MEWMDRMProximityCompleted, L"MEWMDRMProximityCompleted" },
{ MEWMDRMLicenseStoreCleaned, L"MEWMDRMLicenseStoreCleaned" },
{ MEWMDRMRevocationDownloadCompleted, L"MEWMDRMRevocationDownloadCompleted" },
{ METransformUnknown, L"METransformUnknown" },
{ METransformNeedInput ,L"METransformNeedInput"},
{ METransformHaveOutput, L"METransformHaveOutput" },
{ METransformDrainComplete, L"METransformDrainComplete" },
{ METransformMarker, L"METransformMarker" },
{ MEByteStreamCharacteristicsChanged, L"MEByteStreamCharacteristicsChanged" },
{ MEVideoCaptureDeviceRemoved, L"MEVideoCaptureDeviceRemoved" },
{ MEVideoCaptureDevicePreempted, L"MEVideoCaptureDevicePreempted" },
{ MEStreamSinkFormatInvalidated, L"MEStreamSinkFormatInvalidated" },
{ MEEncodingParameters, L"MEEncodingParameters" },
{ MEContentProtectionMetadata, L"MEContentProtectionMetadata" },
{ MEReservedMax, L"MEReservedMax" }
};


void dump_met( const wchar_t*fmt, DWORD met ) {
  for ( auto i = 0; i < _countof( event_names ); i++ ) {
    if ( DWORD(event_names[ i ].id) == met )
      dump( fmt, event_names[ i ].name );
  }
}

void dump_msample( IMFSample* sample ) {
  LONGLONG t = 0, du =0;
  (void)sample->GetSampleDuration( &du );
  (void)sample->GetSampleTime( &t );
  dump( L"sample time: %d ms, duration: %d ms\n", t/10000, du/10000);
}