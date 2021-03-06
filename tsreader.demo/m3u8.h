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
  bool     allow_cache       = true;
  uint64   seqno             = 0;
  string   title;               // optional second parameter for EXTINF tag
  string   uri;                 //absolute
  uint64   duration;            //nano-secs 
  uint64   byte_range_n      = 0;
  uint64   byte_range_offset = 0;
  bool     discontinuity     = false; // EXT-X-DISCONTINUITY 
  // EXT-X-PROGRAM-DATE-TIME tag associates the first sample of a media segment with an absolute date and/or time
};

struct media_playlist{
  uint64     duration        = 0; //second -> 100nano-secs
  uint64     target_duration = 0; // 100 nano-secs
  uint64     seqno           = 0; //EXT-X-MEDIA-SEQUENCE
  bool       iframe          = false; // EXT-X-I-FRAMES-ONLY
  bool       vod             = true; // vod or live else
  bool       allow_cache     = true;
  bool       closed          = false; //HAS ext-x-endlist
  uint       winsize         = 0; // max number of segments displayed in an encoded playlist; 
  uint       ver             = 3; // 
  uint64     resolution_x    = 0;
  uint64     resolution_y    = 0;
  uint64     bandwidth       = 0; //bps
  uint64     program_id      = 1;
  string     key_method;
  string     key_uri;
  string     key_iv;
  string     audio;
  string     video;
  string     codec;             //comma seperated 
  string     uri;               //absolute
  string     title;
  string     playlist_type;    //VOD, EVENT
  std::vector<media_segment> segments;
};

 struct master_playlist{
   uint64                      ver      = 3;
   uint64                      duration = 0;
   int                         error    = 0;
   string                      uri;
   std::vector<media_playlist> medias;
 };
 master_playlist decode_playlist( line_reader*reader , const std::wstring &uri);
 
}
