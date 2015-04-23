#include "pch.h"
#include "attrreader.h"
inline bool is_name_char( char c ) {
  return ( c >= 'A'&&c <= 'Z' ) || c == '-';
}
namespace m3u8 {
//   AttributeName=AttributeValue[,AttributeName=AttributeValue]*
//   Certain extended M3U tags have values which are Attribute Lists.  An
//   Attribute List is a comma-separated list of attribute/value pairs
//   with no whitespace.
struct attrlist_reader {
  int err = 0;                        //-1:eof, 0: ok, other: failed
  int begin = 0;
  const utf8string &line;
  utf8string attrname() { //[A..Z-]
    utf8string n;
    if ( err != 0 ) return n;
    for ( ; begin < line.size() && is_name_char( line[ begin ] ); ++begin );
    return n;
  }
  explicit attrlist_reader( utf8string const&line ) :line( line ) {}
  void expect_eq() {
    if ( begin < line.size() && line[ begin ] == '=' )
      ++begin;
    else err = -2;
    return;
  }
  void expect_comma() {
    if ( err != 0 )return;
    if ( begin < line.size() && line[ begin ] == ',' )
      ++begin;
    else if ( begin < line.size() )
      err = -4;
  }
  utf8string attrvalue() {
    utf8string v;
    if ( err != 0 ) return v;
    if ( line[ begin ] == '"' )
      return string_value();
    for ( ; begin < line.size() && line[ begin ] != ','; ++begin ) {
      v.push_back( line[ begin ] );
    }
    return v;
  }
  utf8string string_value() {
    utf8string v;
    ++begin;  //ignore the first "
    auto x = false;
    for ( ; begin < line.size(); ++begin ) {
      auto c = line[ begin ];
      if ( x == true ) {
        if ( c == 'r' )
          v.push_back( '\r' );
        else if ( c == 'n' )
          v.push_back( '\n' );
        else v.push_back( line[ begin ] );
        x = false;
        continue;
      }
      if ( c == '\\' ) {
        x = true;
        continue;
      }
      if ( c == '"' ) {
        break;
      }
      v.push_back( line[ begin ] );
    }
    if ( begin >= line.size() || line[ begin ] != '"' )
      err = -3;
    ++begin;
    return v;
  }
private:
  attrlist_reader operator=( const attrlist_reader& ) = delete;
};
}
using namespace m3u8;
params_t to_params( utf8string const&line ) {
  params_t v;
  attrlist_reader reader( line );
  while ( reader.err == 0 && reader.begin < line.size() ) {
    auto name = reader.attrname();
    reader.expect_eq();
    auto val = reader.attrvalue();
    if (reader.err == 0 )
      v[ name ] = val;
    reader.expect_comma();
  }
  return v;
}
