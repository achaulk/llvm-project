#pragma once

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

#include "util.h"

#include <Windows.h>

class W32Pipe : public Pipe {
public:
  W32Pipe(HANDLE r, HANDLE w) : r_(r), w_(w) {}
  ~W32Pipe() {
    CloseHandle(r_);
    CloseHandle(w_);
  }
  bool Write(const void *buf, uint32_t n) override {
    DWORD ofs = 0;
    while (ofs < n) {
      DWORD w;
      if (!WriteFile(w_, static_cast<const uint8_t *>(buf) + ofs, n - ofs, &w,
                     NULL) ||
          !w)
        return false;
      ofs += w;
    }
    return true;
  }
  uint32_t Read(void *buf, uint32_t n) override {
    DWORD r;
    if (!ReadFile(r_, buf, n, &r, NULL))
      return 0;
    return r;
  }

  HANDLE r_, w_;
};

std::error_code openNativeFileInternalRedirectImpl(const llvm::Twine &Name,
                                                   HANDLE &ResultFile,
                                                   DWORD Disp, DWORD Access,
                                                   DWORD Flags, bool Inherit);
