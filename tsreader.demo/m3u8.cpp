#include "pch.h"
#include "m3u8.h"
#include <algorithm>
#include <sstream>
#include <ctime>
using namespace m3u8;

const uint64 second = 10000000ull/100; // 100 nano-secs

struct m3u8stack {
  bool                  m3u8                  = false;
  bool                  is_media_list         = false;
  bool                  is_master_list        = false;
  bool                  allow_cache           = true;
  bool                  end_list              = false;
  bool                  discontinuity         = false;
  bool                  iframes_only          = false;
  uint64                duration              = 0; // for media-list
  uint64                inf_duration          = 0;
  uint64                byte_range_n          = 0;
  uint64                byte_range_offset     = 0;
  uint64                target_duration       = 0; //100 nano-secs
  uint64                media_sequence        = 0;
  uint64                program_datetime      = 0; //unixtime64
  uint64                ver                   = 3;
  string                inf_title;
  string                playlist_type = L"VOD"; // "" = VOD, EVENT = LIVE,
  string                uri;
  std::vector<params_t> medias;
  ::media_playlist      media_playlist;
  ::master_playlist     master_playlist;
  params_t              stream_inf;
  params_t              iframe_inf;
  params_t              key;
  int                   error                 = 0;
};

utf8string              suffix( utf8string &line, utf8string const&prefix );
utf8string              trim( utf8string const&line );
utf8string              suffix_trim( utf8string & , utf8string const&prefix);
string                  utf82unicode( utf8string const& );
bool                    has_prefix( utf8string const&line, utf8string const&prefix );
std::vector<utf8string> string_split( utf8string const&line, char sep );

double to_float64( utf8string const&line );  // std::stod
uint64 to_uint64( utf8string const&key );  // std::stoull
params_t to_params( utf8string const&line );

uint64                     as_uint64( params_t const&, utf8string const&key );
std::tuple<uint64, uint64> as_resolution( params_t const&, utf8string const&key );
utf8string                 as_string( params_t const&,utf8string const& key);
utf8string                 as_enum( params_t const&, utf8string const&key, utf8string const&dft );

void           media_playlist_ctor( m3u8stack &ctx );
void           master_playlist_ctor( m3u8stack &ctx );
media_segment  make_segment( m3u8stack &ctx, utf8string const&url );  // apply extinf
media_playlist make_media_playlist(m3u8stack &ctx, utf8string const&url);  // apply ext-x-stream-inf


// n = 0 : means no byte-range,
// offset = -1 means, use previous offset
std::tuple<uint64, uint64> decode_byte_range( utf8string const&line );
uint64                     decode_program_datetime( utf8string const&line );
bool                       decode_yesno( utf8string const&line );
std::tuple<uint64, string> decode_inf( utf8string const&line );  //EXTINF
params_t decode_key( utf8string line );

//#EXT-X-MEDIA-SEQUENCE:<number>
uint64 decode_media_sequence( utf8string const&line ) {
  return to_uint64( line );
}

// #EXT-X-PROGRAM-DATE-TIME:<YYYY-MM-DDThh:mm:ssZ>
uint64 decode_program_datetime( utf8string const&line ) {
  //strptime not found on windows
  line;
  return 0;
}

bool decode_yesno( utf8string const&line ) {
  return trim(line) == "YES";
}

//#EXT-X-TARGETDURATION:<s>
uint64 decode_target_duration( utf8string line ) {
  return to_uint64( line ) * second;  //100 nano-secs
}
//#EXTINF:<duration>,<title> , duration is integer or float number in decimal, seconds
std::tuple<uint64, string> decode_inf( utf8string const&line ) {
  auto fields = string_split( line, ',' );
  uint64 d = 0;
  string t;
  if ( fields.size() >= 2 ) {
    t = utf82unicode( fields[ 1 ] );
  } else if ( fields.size() == 1 ) {
    d = static_cast<uint64>( to_float64( fields[ 0 ] ) * second ); // 100 nano-secs
  }
  return std::make_tuple(d, t);
}

void assure(bool exp) {
  // todo: implement later
  exp;
}

master_playlist m3u8::decode_playlist( line_reader*reader , const std::wstring &uri) {
  const char *t = nullptr;
  m3u8stack ctx;
  ctx.uri = uri;
  auto err = 0;
  auto line = trim(reader->read_line( &err ));
  while ( err >= 0 && ctx.error == 0 ) {
    if ( line.empty() ) {
    } else if ( line == "#EXTM3U" ) {
      ctx.m3u8 = true;
    } else if ( has_prefix( line, t = "#EXTINF:" ) ) {//#EXTINF:<duration>,<title>
      ctx.is_media_list = true;
      std::tie(ctx.inf_duration, ctx.inf_title) = decode_inf( suffix_trim( line, t ) );
      ctx.duration +=ctx.inf_duration;
    } else if ( has_prefix( line, t = "#EXT-X-BYTERANGE:" ) ) {
      ctx.is_media_list = true;
      auto br = decode_byte_range( suffix_trim( line, t ) );
      if ( std::get<1>(br) != 0 - 1ull ) {// offset
        ctx.byte_range_offset = std::get<1>( br );
      }
      ctx.byte_range_n = std::get<0>( br );
    } else if ( has_prefix( line, t = "#EXT-X-TARGETDURATION:" ) ) {//#EXT-X-TARGETDURATION:<s>
      ctx.is_media_list = true;
      ctx.target_duration = decode_target_duration( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-MEDIA-SEQUENCE:" ) ) {//#EXT-X-MEDIA-SEQUENCE:<number>
      ctx.is_media_list = true;
      assure( ctx.media_sequence == 0 );// no media-sequence defined yet
      ctx.media_sequence = decode_media_sequence( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-KEY:" ) ) {
      ctx.key = decode_key( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-PROGRAM-DATE-TIME:" ) ) {//#EXT-X-PROGRAM-DATE-TIME:<YYYY-MM-DDThh:mm:ssZ>
      ctx.is_media_list = true;
      ctx.program_datetime = decode_program_datetime( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-ALLOW-CACHE:" ) ) {// #EXT-X-ALLOW-CACHE:<YES|NO>
      ctx.allow_cache = decode_yesno( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-PLAYLIST-TYPE:" ) ) {
      ctx.playlist_type = utf82unicode(trim( suffix_trim( line, t ) ));
    } else if ( has_prefix( line, t = "#EXT-X-ENDLIST" ) ) {
      ctx.is_media_list = true;
      ctx.end_list = true; // media-playlist
    } else if ( has_prefix( line, t = "#EXT-X-MEDIA:" ) ) {
      ctx.medias.push_back( to_params( suffix_trim( line, t ) ) );
    } else if ( has_prefix( line, t = "#EXT-X-STREAM-INF:" ) ) {  //#EXT-X-STREAM-INF:<attribute-list>
      ctx.is_master_list = true;
      ctx.stream_inf = to_params( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-DISCONTINUITY" ) ) {
      ctx.is_media_list = true;
      ctx.discontinuity = true;
    } else if ( has_prefix( line, t = "#EXT-X-I-FRAMES-ONLY" ) ) {
      ctx.is_media_list = true;
      ctx.iframes_only = true;
    } else if ( has_prefix( line, t = "#EXT-X-I-FRAMES-STREAM-INF:" ) ) {//#EXT-X-I-FRAME-STREAM-INF:<attribute-list>
      ctx.is_media_list = true;
      ctx.iframe_inf = to_params( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-VERSION:" ) ) {//#EXT-X-VERSION:<n>
      ctx.ver = to_uint64( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#" ) ) {
    } else {
      //segment
      //media-playlist
      if ( ctx.is_media_list ) {
        auto p = make_segment( ctx, line);
        ctx.media_playlist.segments.push_back( p );
        ctx.media_sequence++;
      }
      if ( ctx.is_master_list ) {
        auto p = make_media_playlist( ctx, line );
        ctx.master_playlist.medias.push_back( p );
      }
      ctx.byte_range_offset += ctx.byte_range_n;  //media-list
      ctx.byte_range_n = 0;  // media-list
      ctx.discontinuity = false;  // media-list
      ctx.program_datetime = 0;  //unixtimestamp, media-list
    }
    line = trim(reader->read_line( &err ));
  }
  if ( err != -1 ) {  // -1: end of stream
    ctx.error = ctx.master_playlist.error = err;
  }
  master_playlist_ctor( ctx );
  if ( ctx.is_media_list ) {
    media_playlist_ctor( ctx );
    ctx.master_playlist.medias.push_back( ctx.media_playlist );
  }

  return ctx.master_playlist;
}

params_t decode_key( utf8string line ) {
  return to_params( line );
}
// #EXT-X-BYTERANGE:<n>[@o]
std::tuple<uint64, uint64> decode_byte_range( utf8string const&line ) {
  auto offset = 0 - 1ull;
  auto fields = string_split( line, '@' );
  if ( fields.size() == 2 ) {
    offset = to_uint64( fields[ 1 ] );
  }
  auto n = to_uint64( fields[ 0 ] );

  return std::make_tuple( n, offset );
}

string uri_resolve( string const&url, string const&relative );

media_segment make_segment(m3u8stack &ctx, utf8string const&url) {
  media_segment v;
  if ( ctx.byte_range_n > 0 ) {  // apply byte-range
    v.byte_range_n = ctx.byte_range_n;
    v.byte_range_offset = ctx.byte_range_offset;
  }
  v.uri = uri_resolve( ctx.uri, utf82unicode(url) );// may be relative url
  if ( v.uri.empty() )
    ctx.error = -2;  // just a magic number
  v.duration = ctx.inf_duration;
  v.seqno = ctx.media_sequence ;
  v.title = ctx.inf_title;
  v.discontinuity = ctx.discontinuity;
  v.allow_cache = ctx.allow_cache;
  return v;
}

media_playlist make_media_playlist(m3u8stack &ctx, utf8string const&url) {
  media_playlist v;
  v.uri = uri_resolve( ctx.uri, utf82unicode(url) );
  if ( v.uri.empty() )
    ctx.error = -3;
  if ( !ctx.stream_inf.empty() ) {
    //BANDWIDTH decimal-integer bps, PROGRAM-ID decimal-integer
    //, CODECS  quoted-string comma-separated,RESOLUTION \d+ x \d+
    //, AUDIO quoted-string, VIDEO quoted-string
    v.program_id = as_uint64( ctx.stream_inf, "PROGRAM-ID" );
    v.bandwidth = as_uint64( ctx.stream_inf, "BANDWIDTH" );
    std::tie(v.resolution_x, v.resolution_y) = as_resolution( ctx.stream_inf, "RESOLUTION" );
    v.audio = utf82unicode(as_string( ctx.stream_inf, "AUDIO" ));
    v.video = utf82unicode(as_string( ctx.stream_inf, "VIDEO" ));
    v.codec = utf82unicode(as_string( ctx.stream_inf, "CODEC" ));
  }
  return v;
}

void media_playlist_ctor(m3u8stack &ctx) {
  auto &mp = ctx.media_playlist;
  mp.target_duration = ctx.target_duration;
  mp.allow_cache = ctx.allow_cache;
  mp.key_method = utf82unicode( as_enum( ctx.key, "METHDO", "NONE" ) );
  mp.key_uri = utf82unicode( as_string( ctx.key, "URI" ) );
  mp.key_iv = utf82unicode( as_string( ctx.key, "IV" ) );  //128-bit hexadecimal
  mp.closed = ctx.end_list;
  mp.uri = ctx.uri;
  mp.seqno = ctx.media_sequence;  // next seq-no
  mp.playlist_type = ctx.playlist_type;
}

void master_playlist_ctor( m3u8stack &ctx ) {
  auto &mp = ctx.master_playlist;
  mp.error = ctx.error;
  if ( mp.error == 0 && !ctx.m3u8 )
    mp.error = -4;
  mp.ver = ctx.ver;
  mp.uri = ctx.uri;
  mp.duration = ctx.duration;
  //EXT-X-KEY shouldn't exist here

  // todo: #EXT-X-MEDIA:TYPE, i dont know how to process x-media
  // we just abandoned it now
}

utf8string suffix(utf8string &s, utf8string const&prefix){
  return s.substr(prefix.length());
}
utf8string suffix_trim(utf8string &s, utf8string const&prefix){
  auto v = s.substr(prefix.length());
  return trim(v);
}

bool is_not_space(char const&c){
  return !(c == ' ' || c == '\t');
}
utf8string ltrim(utf8string v){
  v.erase(v.begin(), std::find_if(v.begin(), v.end(), is_not_space));
  return std::move(v);
}
utf8string rtrim(utf8string v){
  v.erase((std::find_if(v.rbegin(), v.rend(), is_not_space)).base(), v.end());
  return std::move(v);
}

utf8string trim(utf8string const&s){
  return rtrim(ltrim(s));
}

std::vector<utf8string> string_split(utf8string const&line, char sep){
  std::vector<utf8string> v;
  std::stringstream ss(line);
  utf8string i;
  while(std::getline(ss, i, sep)){
    v.push_back(i);
  }
  return v;
}

string utf82unicode(utf8string const&s){
  std::vector<wchar_t> buf(s.size());
  auto r = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), buf.data(), static_cast<int>(buf.size()));
  return string(buf.data(), r);
}

bool has_prefix(utf8string const&s, utf8string const&prefix){
  auto i = std::mismatch(prefix.begin(), prefix.end(), s.begin());
  return i.first == prefix.end();
}

//throw invalid_argument exception
double to_float64(utf8string const&s){
  return std::stod(s);
}

//throw invalid_argument exception
uint64 to_uint64(utf8string const&s){
  return std::stoull(s);
}

uint64 as_uint64( params_t const&x, utf8string const&key ){
  auto i = x.find(key);
  if (i != x.end()){
    return to_uint64(i->second);
  }
  return 0;
}

utf8string as_enum( params_t const&x, utf8string const&key, utf8string const&dft ){
  auto i = x.find(key);
  return i == x.end() ? dft : i->second;
}
//\d+x\d+ without whitespace
std::tuple<uint64, uint64> as_resolution( params_t const&x, utf8string const&key ){
  auto v = as_enum(x, key, "0x0");
  auto fields = string_split(v, 'x');
  if (fields.size() == 2){
    return std::make_tuple(to_uint64(fields[0]), to_uint64(fields[1]));
  }
  return std::make_tuple(0ull, 0ull);
}

utf8string as_string( params_t const&x, utf8string const& key){
  return as_enum(x, key, "");
}
