#pragma once
#include <string>
namespace m3u8 {
struct __declspec( novtable ) line_reader {
  virtual std::string read_line( int *err ) = 0;
};
struct buffer_reader : line_reader {
  const char* data;
  size_t current = 0;
public:
  explicit buffer_reader( const char*data ): data(data){};
  virtual ~buffer_reader() {}
  std::string read_line(int *err) override{
    std::string v;
    if ( data[ current ] == 0 ) {
      *err = -1;
      return v;
    }
    while ( data[current] && data[current]!='\r' && data[current] != '\n') {
      v.push_back( data[ current ] );
      ++current;
    }
    while ( data[ current ] == '\r' || data[ current ] == '\n' )++current;
    return v;
  }

};
}