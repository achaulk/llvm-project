#pragma once

#include <stdint.h>

#include <memory>

#include "compile_generated.h"

class Pipe {
public:
  virtual ~Pipe() = default;
  virtual bool Write(const void *buf, uint32_t n) = 0;
  virtual uint32_t Read(void *buf, uint32_t n) = 0;
};

Pipe *GetCompilerPipe();
void SetCompilerPipe(Pipe *pipe);

class WriteImpl {
public:
  virtual ~WriteImpl() = default;
  virtual void Write(const char *mem, size_t size, uint64_t offset) = 0;
  virtual void Flush() = 0;
  virtual void *GetNative() = 0;
};

void ResetOutputFiles();
std::shared_ptr<WriteImpl> OpenOutputFile(const char *path);
std::shared_ptr<WriteImpl> CreateOutputFile(const compiler::FileContents &response);

void InitFileOverrides();
