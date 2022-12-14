#include "remote_vfs.h"
#include "remote_driver.h"

#include "llvm/Support/BuryPointer.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/StringSaver.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Timer.h"
#include "llvm/Support/VirtualFileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <Windows.h>
#include <map>

bool ReadWithOffset(HANDLE h, void *mem, size_t size, size_t offset) {
  DWORD n;
  OVERLAPPED ov = {0};
  ov.Pointer = (PVOID)offset;
  if (!ReadFile(h, mem, size, NULL, &ov)) {
    if (GetLastError() != ERROR_IO_PENDING)
      return false;
  }
  if (!GetOverlappedResult(h, &ov, &n, TRUE))
    return false;
  return n == size;
}

class VVFS_RemoteFile : public vfs::File {
public:
  VVFS_RemoteFile(const compiler::OpenResponse *info)
      : name_(info->file()->canonical_filename()->str()),
        h_((HANDLE)(size_t)info->file()->handle()),
        offset_(info->file()->handle_offset()), size_(info->file()->size()),
        uuid_(info->file()->uuid()) {
    if(info->file()->raw_contents())
        contents_ = info->file()->raw_contents()->str();
  }
  ~VVFS_RemoteFile() override {
    if(h_)
        CloseHandle(h_);
  }

  llvm::ErrorOr<Status> status() override {
    return llvm::ErrorOr<Status>(Status(
        name_, sys::fs::UniqueID(1, uuid_), sys::toTimePoint(0), 0, 0, size_,
        sys::fs::file_type::regular_file, sys::fs::perms::all_read));
  }

  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>
  getBuffer(const Twine &Name, int64_t FileSize = -1,
            bool RequiresNullTerminator = true,
            bool IsVolatile = false) override {
    if (!contents_.empty()) {
      return llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>(
          llvm::MemoryBuffer::getMemBufferCopy(StringRef(contents_), name_));
    }

    char *mem = (char *)malloc(size_);
    if (!ReadWithOffset(h_, mem, size_, offset_)) {
      free(mem);
      return llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>(
          std::make_error_code(std::errc::io_error));
    }
    auto ret = llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>>(
        llvm::MemoryBuffer::getMemBufferCopy(StringRef(mem, size_), name_));
    free(mem);
    return ret;
  }

  std::error_code close() override { return std::error_code(); }

  std::string name_;
  std::string contents_;
  HANDLE h_;
  uint64_t offset_;
  uint64_t size_;
  uint64_t uuid_;
};

class VVFS : public vfs::FileSystem {
public:
  VVFS(std::string cwd) : cwd_(std::move(cwd)) {}
  llvm::ErrorOr<Status> status(const Twine &Path) override {
    flatbuffers::FlatBufferBuilder fbb;
    fbb.FinishSizePrefixed(compiler::CreateCompilerMessage(
        fbb, compiler::CompilerToHost_StatRequest,
        compiler::CreateStatRequestDirect(fbb, g_compileID, Path.str().c_str())
            .Union()));
    auto r = g_pump
                 ->TransactOrDie(
                     fbb, compiler::HostToCompiler::HostToCompiler_StatResponse)
                 ->msg_as_StatResponse();
    if (!r->exists())
      return llvm::ErrorOr<Status>(
          std::make_error_code(std::errc::no_such_file_or_directory));
    return Status(Path, llvm::sys::fs::UniqueID(1, r->uuid()),
                  sys::toTimePoint(0), 0, 0, r->size(),
                  r->is_dir() ? sys::fs::file_type::directory_file
                              : sys::fs::file_type::regular_file,
                  r->write()
                      ? sys::fs::perms::all_write | sys::fs::perms::all_read
                      : sys::fs::perms::all_read);
  }

  llvm::ErrorOr<std::unique_ptr<File>>
  openFileForRead(const Twine &Path) override {
    flatbuffers::FlatBufferBuilder fbb;
    fbb.FinishSizePrefixed(compiler::CreateCompilerMessage(
        fbb, compiler::CompilerToHost_OpenRequest,
        compiler::CreateOpenRequestDirect(fbb, g_compileID, Path.str().c_str())
            .Union()));
    auto r = g_pump
                 ->TransactOrDie(
                     fbb, compiler::HostToCompiler::HostToCompiler_OpenResponse)
                 ->msg_as_OpenResponse();
    return std::make_unique<VVFS_RemoteFile>(r);
  }

  directory_iterator dir_begin(const Twine &Dir, std::error_code &EC) override {
    return directory_iterator();
  }

  std::error_code setCurrentWorkingDirectory(const Twine &Path) override {
    cwd_ = Path.str();
    return std::error_code();
  }

  llvm::ErrorOr<std::string> getCurrentWorkingDirectory() const override {
    return llvm::ErrorOr<std::string>(cwd_);
  }

  std::error_code getRealPath(const Twine &Path,
                              SmallVectorImpl<char> &Output) const override {
    return std::error_code();
  }

  std::error_code isLocal(const Twine &Path, bool &Result) override {
    return std::error_code();
  }

  std::error_code makeAbsolute(SmallVectorImpl<char> &Path) const override {
    return std::error_code();
  }

  struct Entry {
    Entry() = default;
    Entry(const char *s) : in(s), sz(strlen(s)) {}
    Entry(const char *s, size_t sz) : in(s), sz(sz) {}
    const char *in;
    size_t sz;
  };

  std::string cwd_;
};

vfs::FileSystem *CreateVFS(std::string cwd) { return new VVFS(std::move(cwd)); }
