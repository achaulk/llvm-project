#pragma once

#include "compile_generated.h"

#include <memory>
#include <vector>

#include "util.h"

template <typename T> class FlatbufferMessagePump {
public:
  FlatbufferMessagePump(Pipe *pipe) : pipe_(pipe) { readbuf_.resize(1024 * 32); }

  bool Write(flatbuffers::FlatBufferBuilder &fbb) {
    return pipe_->Write(fbb.GetBufferPointer(), fbb.GetSize());
  }

  const T *Read() {
    readofs_ += lastsz_;
    lastsz_ = 0;
    while (true) {
      if (readvalid_ >= readofs_ + 4) {
        uint32_t n;
        memcpy(&n, readbuf_.data() + readofs_, 4);
        if (readvalid_ >= n + 4) {
          lastsz_ = n + 4;
          const T *p = flatbuffers::GetSizePrefixedRoot<T>(readbuf_.data() +
                                                     readofs_);
          flatbuffers::Verifier v(readbuf_.data() + readofs_ + 4, n);
          if (p->Verify(v))
            return p;
          abort();
        }
      }
      readvalid_ -= readofs_;
      memmove(readbuf_.data(), readbuf_.data() + readofs_, readvalid_);
      readofs_ = 0;
      uint32_t n = pipe_->Read(readbuf_.data(), readbuf_.size() - readofs_);
      if (!n)
        return nullptr;
      readvalid_ += n;
    }
  }

  std::pair<const T *, std::vector<uint8_t>> CopyCurrent() const {
    std::vector<uint8_t> v;
    v.resize(lastsz_);
    memcpy(v.data(), readbuf_.data() + readofs_, lastsz_);
    const T *p =
        flatbuffers::GetSizePrefixedRoot<T>(v.data());
    return std::make_pair(p, std::move(v));
  }

  template<typename Type>
  const T *TransactOrDie(flatbuffers::FlatBufferBuilder &fbb, Type ty) {
    if (!Write(fbb)) {
      abort();
    }
    const T *ret = Read();
    if (!ret || ty != ret->msg_type())
      abort();
    return ret;
  }

private:
  Pipe *pipe_;
  std::vector<uint8_t> readbuf_;
  uint32_t readofs_ = 0;
  uint32_t readvalid_ = 0;
  uint32_t lastsz_ = 0;
};
