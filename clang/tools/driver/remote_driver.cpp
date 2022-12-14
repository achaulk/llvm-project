#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/HeaderInclude.h"
#include "clang/Basic/Stack.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Config/config.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Options.h"
#include "clang/Driver/Tool.h"
#include "clang/Driver/ToolChain.h"
#include "clang/Frontend/ChainedDiagnosticConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/SerializedDiagnosticPrinter.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/Utils.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Config/config.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/Linker/Linker.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Option/Option.h"
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
#include "llvm/Target/TargetMachine.h"
// #include "lld/Common/Driver.h"

#include "remote_driver.h"
#include "remote_vfs.h"

namespace lld {
namespace elf {
bool link(llvm::ArrayRef<const char *> args, llvm::raw_ostream &stdoutOS,
          llvm::raw_ostream &stderrOS, bool exitEarly, bool disableOutput);
}
} // namespace lld

uint32_t g_compileID = 0;

using namespace clang;
using namespace clang::driver;
using namespace llvm::opt;
using namespace llvm;
using namespace llvm::vfs;

class streamwrite_buffer : public raw_ostream {
public:
  streamwrite_buffer() {}
  ~streamwrite_buffer() {}

  void write_impl(const char *Ptr, size_t Size) override {
    buf.append(Ptr, Size);
  }
  uint64_t current_pos() const override { return 0; }

  std::string buf;
};

bool CompileObj(llvm::opt::ArgStringList &args, DiagnosticsEngine &Diags,
                raw_ostream &g_stderr,
                IntrusiveRefCntPtr<llvm::vfs::FileSystem> vfs,
                clang::FrontendAction &action,
                std::unique_ptr<llvm::raw_pwrite_stream> output,
                const caas::CompileRequest &req) {
  std::unique_ptr<CompilerInvocation> CI(new CompilerInvocation);
  CI->TargetOpts->Triple;
  CompilerInvocation::CreateFromArgs(*CI, args, Diags);

  CompilerInstance Clang;
  Clang.setInvocation(std::move(CI));
  Clang.setVerboseOutputStream(g_stderr);

  Clang.createDiagnostics(Diags.getClient(), false);
  if (!Clang.hasDiagnostics())
    return false;

  Clang.createFileManager(vfs);

  auto &hdr_opts = Clang.getHeaderSearchOpts();
  hdr_opts.UserEntries.clear();
  hdr_opts.UseStandardSystemIncludes = 0;
  hdr_opts.UseStandardCXXIncludes = 0;
  hdr_opts.UseBuiltinIncludes = 0;
  hdr_opts.Sysroot = "/";
  for (auto &s : req.sys_includes)
    hdr_opts.SystemHeaderPrefixes.emplace_back(s.c_str(), true);
  for (auto &s : req.user_includes)
    hdr_opts.SystemHeaderPrefixes.emplace_back(s.c_str(), false);

  Clang.setOutputStream(std::move(output));

  return Clang.ExecuteAction(action);
}

void ResetOutputFiles() {}

std::shared_ptr<WriteImpl> OpenOutputFile(const char *path) {
  caas::OpenResponse resp;
  g_msgpump->TransactOrDie(caas::OpenRequest{g_compileID, path, true, true},
                           resp);

  if (resp.error != caas::FileError::None)
    return nullptr;
  auto v = CreateOutputFile(resp.contents);
  return v;
}

class ostream_impl : public raw_pwrite_stream {
public:
  ostream_impl(std::shared_ptr<WriteImpl> writer) : writer_(writer) {}
  ~ostream_impl() {
    flush();
    if (writer_)
      writer_->Flush();
  }

  void write_impl(const char *Ptr, size_t Size) override {
    pwrite_impl(Ptr, Size, pos_);
    pos_ += Size;
  }
  uint64_t current_pos() const override { return pos_; }
  void pwrite_impl(const char *Ptr, size_t Size, uint64_t Offset) override {
    if (writer_)
      writer_->Write(Ptr, Size, Offset);
  }

private:
  uint64_t pos_ = 0;
  std::shared_ptr<WriteImpl> writer_;
};

void DoCompile(streamwrite_buffer &action_stderr,
               const caas::CompileRequest &req, caas::CompileStatus &status) {
  llvm::opt::ArgStringList compile_args;
  llvm::opt::ArgStringList link_args;
  std::unique_ptr<clang::FrontendAction> action;

  // lld expects arg 0 to be the exe name
  link_args.push_back("clang-ipc");

  for (auto &arg : req.cc_opts)
    compile_args.push_back(arg.c_str());
  for (auto &arg : req.ld_opts)
    link_args.push_back(arg.c_str());

  bool compile = false;
  bool link = false;

  switch (req.mode) {
  case caas::CompileMode::Preprocess:
    compile = true;
    break;
  case caas::CompileMode::CompileAndLink:
    compile = true;
    link = true;
    link_args.push_back("-o");
    link_args.push_back(req.output_binary.c_str());
    action.reset(new EmitBCAction());
    break;
  case caas::CompileMode::CompileToAssembly:
    compile = true;
    action.reset(new EmitAssemblyAction());
    break;
  case caas::CompileMode::CompileToObj:
    compile = true;
    action.reset(new EmitObjAction());
    break;
  default:
    abort();
  }

  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
  TextDiagnosticPrinter *DiagClient =
      new TextDiagnosticPrinter(action_stderr, &*DiagOpts);
  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);

  IntrusiveRefCntPtr<llvm::vfs::FileSystem> vfs = CreateVFS(req.cwd);

  for (auto &src : req.srcs) {
    if (src.link) {
      if (src.compile) {
        link_args.push_back(src.obj_path.c_str());
      } else {
        link_args.push_back(src.canonical_filename.c_str());
        continue;
      }
    }
    llvm::opt::ArgStringList t_args = compile_args;
    t_args.push_back(src.canonical_filename.c_str());

    std::shared_ptr<WriteImpl> writer = OpenOutputFile(src.obj_path.c_str());

    if (!CompileObj(t_args, Diags, action_stderr, vfs, *action.get(),
                    std::make_unique<ostream_impl>(std::move(writer)), req)) {
      status.error = std::string("Compile failure: ") + src.canonical_filename;
      return;
    }
  }
  if (link) {
    for (auto &arg : req.late_ld_opts)
      link_args.push_back(arg.c_str());
    if (!lld::elf::link(link_args, action_stderr, action_stderr, false,
                        false)) {
      status.error = "Link failure";
      return;
    }
  }
}

CompilerPump *g_msgpump;

void CompileAction(const caas::CompileRequest &req) {
  g_compileID = req.id;
  streamwrite_buffer action_stderr;
  caas::CompileStatus status;
  DoCompile(action_stderr, req, status);
  action_stderr.flush();
  ResetOutputFiles();

  g_msgpump->Write(status);
}

void run_driver() {
  caas::HostToCompiler m;
  while (true) {
    if (!g_msgpump->Pump())
      return;
    while (true) {
      auto s = g_msgpump->Read(m);
      if (s == caas::ReadStatus::NoData)
        break;
      if (s == caas::ReadStatus::Invalid)
        return;
      if (std::holds_alternative<caas::CompileRequest>(m.msg)) {
        CompileAction(std::get<caas::CompileRequest>(m.msg));
      } else if (std::holds_alternative<caas::Exit>(m.msg)) {
        return;
      } else {
        // Unknown!
      }
    }
  }
}

static constexpr uint32_t kIPCVersion = 1;

void send_hello() {
  std::vector<std::string> targets;
  for (const auto &T : TargetRegistry::targets()) {
    targets.push_back(T.getName());
  }
  g_msgpump->Write(
      caas::CompilerInfo{kIPCVersion, sys::getDefaultTargetTriple(),
                         sys::getHostCPUName().str(), PACKAGE_STRING, targets});
}

void remote_driver(int argc, char **argv) {
  noteBottomOfStack();

#if _DEBUG
  test_compile();
#endif

  CompilerPump pump(GetCompilerPipe());
  g_msgpump = &pump;

  send_hello();

  InitFileOverrides();

  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();
  run_driver();
  llvm::llvm_shutdown();
}
