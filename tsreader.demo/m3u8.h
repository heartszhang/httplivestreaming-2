#pragma once
#include <vector>
#include <map>
#include <cstdint>
#include "linereader.h"

namespace m3u8 {
using uint = uint32_t;
using string = std::wstring;
using uint64 = uint64_t;
using utf8string = std::string;
using params_t = std::map < utf8string, utf8string > ;

// This structure represents a media segment included in a media playlist.
// Media segment may be encrypted.
struct media_segment {
  bool   allow_cache = true;
  uint64 seqno;
  string title;// optional second parameter for EXTINF tag
  string uri;  //absolute
  uint64 duration;        //nano-secs        // first parameter for EXTINF tag, integer or float
  uint64  limit = 0-1ull; // EXT-X-BYTERANGE <n> is length in bytes for the file under URI
  uint64  offset = 0;     // EXT-X-BYTERANGE [@o] is offset from the start of the file under URI
  uint64  byte_range_n = 0;
  uint64  byte_range_offset = 0;
  params_t key;            // EXT-X-KEY displayed before the segment and means changing of encryption key (in theory each segment may have own key)
  bool   discontinuity = false; // EXT-X-DISCONTINUITY indicates an encoding discontinuity between the media segment that follows 
  // EXT-X-PROGRAM-DATE-TIME tag associates the first sample of a media segment with an absolute date and/or time
};

struct media_playlist{
  uint64   duration;  //second -> 100nano-secs
  uint64   target_duration;  // 100 nano-secs
  uint64   seqno;     //EXT-X-MEDIA-SEQUENCE
  bool     iframe;    // EXT-X-I-FRAMES-ONLY
  bool     vod;       // vod or live else
  bool     has_key;
  bool     has_map;
  bool     allow_cache = true;
  bool     closed = true;//HAS ext-x-endlist
  uint     winsize;   // max number of segments displayed in an encoded playlist; need set to zero for VOD playlists
  uint     count;     // number of segments added to the playlist
  uint     ver;       // 
  uint64    resolution_x;
  uint64    resolution_y;
  uint64    bandwidth;  //bps
  uint64    program_id;
  string    key_method;
  string    key_uri;
  string    key_iv;
  string    audio;
  string    video;
  string    codec;//comma seperated 
  params_t  key;       // EXT-X-KEY is optional encryption key displayed before any segments (default key for the playlist)
  string   uri;  //absolute
  string    title;
  utf8string playlist_type = "VOD";//VOD, EVENT
  std::vector<media_segment> segments;
};

 struct master_playlist{
   uint64     ver;
   string     uri;
   uint64     duration;
   int        error = 0;
   std::vector<media_playlist> medias;
 };
 master_playlist decode_playlist( line_reader*reader );
 
}
