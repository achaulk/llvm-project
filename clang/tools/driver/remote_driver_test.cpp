#include "remote_driver.h"

#include "win32.h"

#include <map>
#include <thread>

std::unique_ptr<FlatbufferMessagePump<compiler::CompilerMessage>>
    test_host_pump;

struct Source {
  Source() = default;
  Source(const char *filename, const char *contents)
      : filename(filename), contents(contents) {}
  std::string filename;
  std::string contents;
};

wchar_t g_temp_path[MAX_PATH + 1];
uint32_t g_temp_path_offset = 0;
uint32_t g_temp_path_seq = 0;
HANDLE CreateAnonymousFile() {
  if (!g_temp_path_offset) {
    g_temp_path_offset = GetTempPathW(MAX_PATH, g_temp_path);

    DWORD pid = GetCurrentProcessId();
    wsprintf(g_temp_path + g_temp_path_offset, L"\\clang.%d.", pid);

    g_temp_path_offset = lstrlen(g_temp_path);
  }

  for (int i = 0; i < 100; i++) {
    wsprintf(g_temp_path + g_temp_path_offset, L"%X", g_temp_path_seq++);
    HANDLE h = CreateFileW(
        g_temp_path, FILE_READ_DATA | FILE_WRITE_DATA, 0, NULL, CREATE_NEW,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (h != INVALID_HANDLE_VALUE)
      return h;
  }
  return NULL;
}

class VFS {
public:
  void stat(const compiler::StatRequest &r) {
    flatbuffers::FlatBufferBuilder fbb;
    flatbuffers::Offset<compiler::StatResponse> v = do_stat(fbb, r);
    fbb.FinishSizePrefixed(compiler::CreateHostMessage(
        fbb, compiler::HostToCompiler_StatResponse, v.Union()));
    test_host_pump->Write(fbb);
  }

  flatbuffers::Offset<compiler::StatResponse>
  do_stat(flatbuffers::FlatBufferBuilder &fbb, const compiler::StatRequest &r) {
    bool exists = false;
    bool is_dir = false;
    bool writable = false;
    uint64_t size = 0;
    uint64_t uuid = 0;
    compiler::FileError err = compiler::FileError_NotExist;

    auto lower_it = files.lower_bound(r.path()->str());
    if (lower_it != files.end()) {
      const char *s_ptr = r.path()->c_str();
      const char *l_ptr = lower_it->first.c_str();
      while (*s_ptr && *s_ptr == *l_ptr)
        s_ptr++, l_ptr++;
      if (*s_ptr == *l_ptr) {
        exists = true;
        if (lower_it->second.h) {
          writable = true;
          size = GetFileSize(lower_it->second.h, NULL);
        } else {
          size = (uint32_t)lower_it->second.contents.size();
        }
      } else if (!*s_ptr && *l_ptr == '/') {
        exists = true;
        is_dir = true;
      } else {
        err = compiler::FileError_NotExist;
      }
    }

    return compiler::CreateStatResponse(fbb, exists, is_dir, writable, size,
                                        uuid, err);
  }

  void open(const compiler::OpenRequest &r) {
    flatbuffers::FlatBufferBuilder fbb;
    flatbuffers::Offset<compiler::OpenResponse> v = do_open(fbb, r);
    fbb.FinishSizePrefixed(compiler::CreateHostMessage(
        fbb, compiler::HostToCompiler_OpenResponse, v.Union()));
    test_host_pump->Write(fbb);
  }

  flatbuffers::Offset<compiler::OpenResponse>
  do_open(flatbuffers::FlatBufferBuilder &fbb, const compiler::OpenRequest &r) {
    auto it = files.find(r.path()->str());
    bool is_obj_dir = r.path()->string_view().substr(0, 3) == "/o/";
    if (it == files.end()) {
      if (r.create()) {
        if (is_obj_dir) {
          HANDLE h = CreateAnonymousFile();
          auto &e = files[r.path()->str()];
          e.uuid = ++uuid;
          DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &e.h, 0,
                          0, DUPLICATE_SAME_ACCESS);
          return compiler::CreateOpenResponse(
              fbb, 0,
              compiler::CreateFileContentsDirect(fbb, r.path()->c_str(), 0,
                                                 (uint32_t)(size_t)h, 0, 0,
                                                 e.uuid));
        }
        return compiler::CreateOpenResponse(
            fbb, 0, 0, compiler::FileError_AccessViolation);
      }
      return compiler::CreateOpenResponse(fbb, 0, 0,
                                          compiler::FileError_NotExist);
    }
    if (r.write() && !is_obj_dir) {
      return compiler::CreateOpenResponse(fbb, 0, 0,
                                          compiler::FileError_AccessViolation);
    }
    flatbuffers::Offset<compiler::FileContents> contents;
    if (it->second.contents.empty()) {
      HANDLE h;
      DuplicateHandle(GetCurrentProcess(), it->second.h, GetCurrentProcess(),
                      &h, 0, 0, DUPLICATE_SAME_ACCESS);
      contents = compiler::CreateFileContentsDirect(
          fbb, it->first.c_str(), 0, (uint32_t)(size_t)h, 0,
          GetFileSize(h, NULL), it->second.uuid);
    } else {
      contents = compiler::CreateFileContentsDirect(
          fbb, it->first.c_str(), it->second.contents.c_str(), 0, 0,
          it->second.contents.size(), it->second.uuid);
    }
    return compiler::CreateOpenResponse(fbb, 0, contents);
  }

  void AddSource(const char *filename, const char *contents) {
    files[filename].contents = contents;
    files[filename].uuid = ++uuid;
  }

  struct File {
    ~File() {
      if (h)
        CloseHandle(h);
    }
    std::string contents;
    HANDLE h = nullptr;
    uint64_t uuid = 0;
  };
  std::map<std::string, File> files;
  uint64_t uuid = 0;
};

const compiler::CompileStatus *
do_compile(VFS *vfs, FlatbufferMessagePump<compiler::CompilerMessage> *pump,
           const std::vector<Source> &sources,
           const std::vector<std::string> &compile_opts,
           const std::vector<std::string> &link_opts,
           compiler::CompileMode mode, uint32_t id = 0) {
  flatbuffers::FlatBufferBuilder fbb;
  std::vector<flatbuffers::Offset<flatbuffers::String>> compile_opts_fb,
      link_opts_fb, late_link_opts_fb;
  std::vector<flatbuffers::Offset<compiler::CompileSourceFile>> sources_fb;
  for (auto &s : compile_opts)
    compile_opts_fb.push_back(fbb.CreateString(s));
  for (auto &s : link_opts)
    link_opts_fb.push_back(fbb.CreateString(s));
  for (auto &s : sources) {
    std::string obj = "/o" + s.filename + ".obj";
    sources_fb.push_back(compiler::CreateCompileSourceFile(
        fbb, fbb.CreateString(s.filename), fbb.CreateString(obj), true, true));
  }
  fbb.FinishSizePrefixed(compiler::CreateHostMessage(
      fbb, compiler::HostToCompiler_CompileRequest,
      compiler::CreateCompileRequestDirect(fbb, &compile_opts_fb, &link_opts_fb,
                                           &late_link_opts_fb,
                                           &sources_fb, mode, id, "/s",
                                           "/o/a.out")
          .Union()));

  if (!pump->Write(fbb))
    abort();
  while (auto m = pump->Read()) {
    switch (m->msg_type()) {
    case compiler::CompilerToHost_CompileStatus:
      return m->msg_as_CompileStatus();
    case compiler::CompilerToHost_OpenRequest:
      vfs->open(*m->msg_as_OpenRequest());
      break;
    case compiler::CompilerToHost_StatRequest:
      vfs->stat(*m->msg_as_StatRequest());
      break;
    default:
      break;
    }
  }
  abort();
}

#include <Windows.h>

#include <optional>

std::unique_ptr<Pipe> test_host_pipe, test_compiler_pipe;

std::pair<std::string, std::string>
do_compile_env(const std::vector<Source> &sources,
               std::vector<std::string> compile_opts,
               std::vector<std::string> link_opts, compiler::CompileMode mode) {
  compile_opts.push_back("-triple");
  compile_opts.push_back("riscv32-unknown-elf");
  compile_opts.push_back("-ffreestanding");
  compile_opts.push_back("-nostdinc++");
  link_opts.push_back("some_app");
  link_opts.push_back("--nostdlib");
  link_opts.push_back("-melf32lriscv");
  VFS vfs;
  for (auto &s : sources)
    vfs.AddSource(s.filename.c_str(), s.contents.c_str());
  auto status = do_compile(&vfs, test_host_pump.get(), sources, compile_opts,
                           link_opts, mode);
  std::string err, out;
  if (status->errored_state())
    err = status->errored_state()->str();
  if (status->output())
    out = status->output()->str();
  return std::make_pair(err, out);
}

void test_compile1() {
  std::vector<std::string> compile_opts;
  std::vector<std::string> link_opts;
  std::vector<Source> sources;

  sources.emplace_back("/s/main.cc", R"(
int main() { return 0; }
)");

  auto err = do_compile_env(sources, compile_opts, link_opts,
                 compiler::CompileMode_CompileAndLink);
}

void test_compile2() {
  std::vector<std::string> compile_opts;
  std::vector<std::string> link_opts;
  std::vector<Source> sources;

  sources.emplace_back("/s/main.cc", R"(
int main() { return "errored"; }
)");

  auto err = do_compile_env(sources, compile_opts, link_opts,
                 compiler::CompileMode_CompileAndLink);
}

void test_compile3() {
  std::vector<std::string> compile_opts;
  std::vector<std::string> link_opts;
  std::vector<Source> sources;

  sources.emplace_back("/s/main.cc", R"(
int f() { return 0; }
)");

  auto err = do_compile_env(sources, compile_opts, link_opts,
                 compiler::CompileMode_CompileAndLink);
}

void test_compile_thread() {
  //test_compile1();
  test_compile2();
  test_compile3();
  ;
}

void test_compile() {
  HANDLE r[2], w[2];
  if (!CreatePipe(&r[0], &w[0], NULL, 0))
    abort();
  if (!CreatePipe(&r[1], &w[1], NULL, 0))
    abort();

  test_host_pipe.reset(new W32Pipe(r[0], w[1]));
  test_compiler_pipe.reset(new W32Pipe(r[1], w[0]));

  test_host_pump =
      std::make_unique<FlatbufferMessagePump<compiler::CompilerMessage>>(
          test_host_pipe.get());
  SetCompilerPipe(test_compiler_pipe.get());

  std::thread(test_compile_thread).detach();
}
