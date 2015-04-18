#include "pch.h"
#include "mp2t.h"
#include "mp2tstream.h"
#include <cassert>

StreamVector::~StreamVector() {
  Clear();
}
size_t StreamVector::GetCount()const { return streams.size(); }
void StreamVector::Clear() {
  streams.clear();
}

HRESULT StreamVector::AddStream(BYTE id, Mp2tStream *pStream) {
  streams.push_back(ComPtr<Mp2tStream>(pStream));
  //streams[count] = pStream;
  //ids[count] = id;
  ids.push_back(id);
  //++count;

  return S_OK;
}

ComPtr<Mp2tStream> StreamVector::Find(BYTE id)const {
  for (auto i = 0; i < streams.size(); i++) {
    if (ids[i] == id) {
      return streams[i];
    }
  }
  return ComPtr<Mp2tStream>();
}

// Accessor.
ComPtr<Mp2tStream>& StreamVector::operator[](DWORD index) {
  assert(index < GetCount());
  return streams[index];
}

// Const accessor.
const ComPtr<Mp2tStream>& StreamVector::operator[](DWORD index) const {
  assert(index < GetCount());
  return streams[index];
}
ComPtr<IMFMediaBuffer> CreateBuffer(DWORD len) {
  ComPtr<IMFMediaBuffer> ret;
  auto hr = MFCreateMemoryBuffer(len, &ret);
  hr;// ignore ret value;
  return ret;
}