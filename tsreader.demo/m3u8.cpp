#include "pch.h"
#include "m3u8.h"

using namespace m3u8;

const uint64 second = 10000000/100ull; // 100 nano-secs

struct m3u8stack {
  bool m3u8 = false;
  bool is_media_list = false;
  bool is_master_list = true;
  bool allow_cache = true;
  bool end_list = false;
  bool discontinuity = false;
  bool iframes_only = false;
  uint64 duration = 0; // for media-list
  uint64 inf_duration = 0;
  uint64 byte_range_n = 0;
  uint64 byte_range_offset = 0;
  uint64 target_duration = 0;  //100 nano-secs
  uint64 media_sequence = 0;
  uint64 program_datetime = 0;  //unixtime64
  uint64 ver = 3;
  string inf_title;
  utf8string playlist_type ; // "" = VOD, EVENT = LIVE, 
  string uri;
  std::vector<params_t> medias;
  ::media_playlist media_playlist;
  ::master_playlist master_playlist;
  params_t  stream_inf;
  params_t  iframe_inf;
  params_t key;
  int error = 0;
};

utf8string suffix_trim( utf8string const& , utf8string const&prefix);
utf8string trim( utf8string const&line );
bool has_prefix( utf8string const&line, utf8string const&prefix );
std::vector<utf8string> string_split( utf8string const&line, char sep );
string utf82unicode( utf8string const& );
double to_float64( utf8string const&line );
utf8string suffix( utf8string const&line, utf8string const&prefix );
uint64 to_uint64( utf8string const&key );
uint64 as_uint64( params_t const&, utf8string const&key );
std::tuple<uint64, uint64> as_resolution( params_t const&, utf8string const&key );
utf8string as_string( params_t const&,utf8string const& key);
utf8string as_enum( params_t const&, utf8string const&key, utf8string const&dft );
void media_playlist_ctor( m3u8stack &ctx );
void master_playlist_ctor( m3u8stack &ctx );
media_segment make_segment( m3u8stack &ctx, utf8string const&url );
media_playlist make_media_playlist(m3u8stack &ctx, utf8string const&url);

std::tuple<uint64, string> decode_inf( utf8string const&line );
std::tuple<uint64, uint64> decode_byte_range( utf8string const&line ); // n = 0 : means no byte-range, offset = -1 means, use previous offset
// uint64 decode_media_sequence( utf8string const&line );
uint64 decode_program_datetime( utf8string const&line );
bool   decode_yesno( utf8string const&line );
void media_playlist_ctro( m3u8stack &ctx );

//#EXT-X-MEDIA-SEQUENCE:<number>
uint64 decode_media_sequence( utf8string const&line ) {
  return to_uint64( line );
}

// #EXT-X-PROGRAM-DATE-TIME:<YYYY-MM-DDThh:mm:ssZ>
uint64 decode_program_datetime( utf8string const&line ) {
  return 0; // not implemented yet
  //return to_unixtime( line );
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
    d = static_cast<uint64>( to_float64( fields[ 0 ] ) * 10000 ); // 100 nano-secs
  }
  return std::make_tuple(d, t);
}

void assure(bool exp) {
  
}
master_playlist m3u8::decode_playlist( line_reader*reader ) {
  const char *t = nullptr;
  m3u8stack ctx;
  auto err = 0;
  auto line = trim(reader->read_line( &err ));
  while ( err >= 0 && ctx.error == 0 ) {
    if ( line.empty() )continue;
    if ( line == "#EXTM3U" ) {
      ctx.m3u8 = true;
    } else if ( has_prefix( line, t = "#EXTINF:" ) ) {//#EXTINF:<duration>,<title>
      ctx.is_media_list = true;
      std::tie(ctx.inf_duration, ctx.inf_title) = decode_inf( suffix_trim( line, t ) );
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
      ctx.playlist_type = trim( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-ENDLIST" ) ) {
      ctx.is_media_list = true;
      ctx.end_list = true; // media-playlist 
    } else if ( has_prefix( line, t = "#EXT-X-MEDIA:" ) ) {
      ctx.medias.push_back( to_params2( suffix_trim( line, t ) ) );
    } else if ( has_prefix( line, t = "#EXT-X-STREAM-INF:" ) ) {  //#EXT-X-STREAM-INF:<attribute-list>
      ctx.is_master_list = true;
      ctx.stream_inf = to_params2( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-DISCONTINUITY" ) ) {
      ctx.is_media_list = true;
      ctx.discontinuity = true;
    } else if ( has_prefix( line, t = "#EXT-X-I-FRAMES-ONLY" ) ) {
      ctx.is_media_list = true;
      ctx.iframes_only = true;
    } else if ( has_prefix( line, t = "#EXT-X-I-FRAMES-STREAM-INF:" ) ) {//#EXT-X-I-FRAME-STREAM-INF:<attribute-list>
      ctx.is_media_list = true;
      ctx.iframe_inf = to_params2( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#EXT-X-VERSION:" ) ) {//#EXT-X-VERSION:<n>
      ctx.ver = to_uint64( suffix_trim( line, t ) );
    } else if ( has_prefix( line, t = "#" ) ) {
    } else {
      //segment
      //media-playlist
      if ( ctx.is_media_list ) {
        auto p = make_segment( ctx, line );
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

//a flaw implementation
params_t m3u8::to_params2( utf8string const&line ) {
  params_t v;
  auto fields = string_split( line, ',' );
  for ( auto i = fields.begin(); i != fields.end(); ++i ) {
    auto nv = string_split( *i, '=' );
    if ( nv.size() != 2 )
      continue;
    v[ trim( nv[ 0 ] ) ] = nv[ 1 ];
  }
  return v;
}

params_t decode_key( utf8string line ) {
  return to_params2( line );
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
  ctx.duration += v.duration;
  v.seqno = ctx.media_sequence;
  v.title = ctx.inf_title;
  v.discontinuity = ctx.discontinuity;
  v.allow_cache = ctx.allow_cache;
  return;
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
  mp.closed = ctx.end_list;
  mp.target_duration = ctx.target_duration;
  mp.allow_cache = ctx.allow_cache;
  mp.key_method = utf82unicode( as_enum( ctx.key, "METHDO", "NONE" ) );
  mp.key_uri = utf82unicode( as_string( ctx.key, "URI" ) );
  mp.key_iv = utf82unicode( as_string( ctx.key, "IV" ) );  //128-bit hexadecimal
  mp.closed = ctx.end_list;
//  mp.seqno = ctx.media_sequence;
}

void master_playlist_ctor( m3u8stack &ctx ) {
  auto &mp = ctx.master_playlist;
  if ( !ctx.m3u8 )
    mp.error = -4;
  //EXT-X-KEY shouldn't exist here

  // todo: #EXT-X-MEDIA:TYPE, i dont know how to process x-media
  // we just abandoned it now
}