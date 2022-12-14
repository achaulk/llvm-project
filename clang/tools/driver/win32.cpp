#include "win32.h"
#include "remote_driver.h"

Pipe *g_compiler_pipe;

Pipe *GetCompilerPipe() {
  if (!g_compiler_pipe) {
    HANDLE r = (HANDLE)4;
    HANDLE w;
    DWORD n;
    if(!ReadFile(r, &w, sizeof(w), &n, NULL) || n != sizeof(w))
        abort();
    static W32Pipe g_static_pipe(r, w);
    g_compiler_pipe = &g_static_pipe;
  }
  return g_compiler_pipe;
}
void SetCompilerPipe(Pipe *pipe) { g_compiler_pipe = pipe; }

struct FileProxy : public WriteImpl {
  FileProxy(HANDLE h) : h_(h) {}
  ~FileProxy() {
    if (h_)
      CloseHandle(h_);
  }

  FileProxy(FileProxy &&o) noexcept
      : h_(o.h_), should_flush_(o.should_flush_), id_(id_) {
    o.h_ = nullptr;
  }
  void operator=(FileProxy &&o) noexcept {
    if (h_)
      CloseHandle(h_);
    h_ = o.h_;
    o.h_ = nullptr;
    should_flush_ = o.should_flush_;
    id_ = o.id_;
  }

  void Write(const char *mem, size_t size, uint64_t offset) override {
    DWORD n;
    OVERLAPPED ov;
    ZeroMemory(&ov, sizeof(ov));
    ov.Pointer = (PVOID)offset;
    if (WriteFile(h_, mem, (DWORD)size, NULL, &ov))
      GetOverlappedResult(h_, &ov, &n, TRUE);
  }
  size_t Read(char *mem, size_t size, uint64_t offset) {
    DWORD n;
    if (!ReadFile(h_, mem, (DWORD)size, &n, NULL))
      return 0;
    return n;
  }
  void Flush() override {
    if (should_flush_)
      FlushFileBuffers(h_);
  }

  void *GetNative() override { return h_; }

  void TakeHandle(HANDLE *h) {
    if (h_)
      SetFilePointer(h_, 0, nullptr, FILE_BEGIN);
    *h = h_;
    h_ = NULL;
  }

  HANDLE h_;
  bool should_flush_ = false;
  int id_ = -1;
};


std::shared_ptr<WriteImpl>
CreateOutputFile(const compiler::FileContents &response) {
  if (!response.handle() || response.handle_offset() != 0) {
    abort();
  }
  return std::make_shared<FileProxy>((HANDLE)(size_t)response.handle());
}

std::error_code openNativeFileInternalRedirectImpl(const llvm::Twine &Name,
                                                   HANDLE &ResultFile,
                                                   DWORD Disp, DWORD Access,
                                                   DWORD Flags, bool Inherit) {
  bool write = (Access != GENERIC_READ);
  bool create = (Disp == CREATE_ALWAYS || Disp == CREATE_NEW || Disp == OPEN_ALWAYS);
  flatbuffers::FlatBufferBuilder fbb;
  fbb.FinishSizePrefixed(compiler::CreateCompilerMessage(
      fbb, compiler::CompilerToHost_OpenRequest,
      compiler::CreateOpenRequestDirect(fbb, g_compileID, Name.str().c_str(),
                                        write, create)
          .Union()));
  auto r = g_pump
               ->TransactOrDie(
                   fbb, compiler::HostToCompiler::HostToCompiler_OpenResponse)
               ->msg_as_OpenResponse();
  switch (r->access_error()) {
  case compiler::FileError_AccessViolation:
    return std::make_error_code(std::errc::permission_denied);
  case compiler::FileError_NotExist:
    return std::make_error_code(std::errc::no_such_file_or_directory);
  default:
    break;
  }

  ResultFile = (HANDLE)(size_t)r->file()->handle();
  return std::error_code();
}

void InitFileOverrides() {
  llvm::sys::path::SetNativeOverrides(&openNativeFileInternalRedirectImpl);
}