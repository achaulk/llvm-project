#include "remote_driver.h"

#include "win32.h"

#include <map>
#include <thread>

caas::MessagePump<caas::HostToCompiler, caas::CompilerToHost, Pipe>
    *test_host_pump;

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
  void stat(const caas::StatRequest &r) {
    caas::StatResponse x{};

    auto lower_it = files.lower_bound(r.path);
    if (lower_it != files.end()) {
      const char *s_ptr = r.path.c_str();
      const char *l_ptr = lower_it->first.c_str();
      while (*s_ptr && *s_ptr == *l_ptr)
        s_ptr++, l_ptr++;
      if (*s_ptr == *l_ptr) {
        x.exists = true;
        if (lower_it->second.h) {
          x.writable = true;
          x.size = GetFileSize(lower_it->second.h, NULL);
        } else {
          x.size = (uint32_t)lower_it->second.contents.size();
        }
      } else if (!*s_ptr && *l_ptr == '/') {
        x.exists = true;
        x.is_dir = true;
      } else {
        x.error = caas::FileError::NotExist;
      }
    }
    x.uuid = uuid;
    test_host_pump->Write(std::move(x));
  }

  void open(const caas::OpenRequest &r) { test_host_pump->Write(do_open(r)); }

  caas::OpenResponse do_open(const caas::OpenRequest &r) {
    auto it = files.find(r.path);
    bool is_obj_dir = r.path.starts_with("/o/");
    if (it == files.end()) {
      if (r.create) {
        if (is_obj_dir) {
          HANDLE h = CreateAnonymousFile();
          auto &e = files[r.path];
          e.uuid = ++uuid;
          DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &e.h, 0,
                          0, DUPLICATE_SAME_ACCESS);
          return caas::OpenResponse{
              0,
              caas::FileError::None,
              {r.path, std::string(), (uint32_t)(size_t)h, 0, 0, e.uuid}};
        }
        return caas::OpenResponse{0, caas::FileError::AccessViolation};
      }
      return caas::OpenResponse{0, caas::FileError::NotExist};
    }
    if (r.write && !is_obj_dir) {
      return caas::OpenResponse{0, caas::FileError::AccessViolation};
    }
    if (it->second.contents.empty()) {
      HANDLE h;
      DuplicateHandle(GetCurrentProcess(), it->second.h, GetCurrentProcess(),
                      &h, 0, 0, DUPLICATE_SAME_ACCESS);
      return caas::OpenResponse{0,
                                caas::FileError::None,
                                {r.path, std::string(), (uint32_t)(size_t)h, 0,
                                 GetFileSize(h, NULL), it->second.uuid}};
    } else {
      return caas::OpenResponse{0,
                                caas::FileError::None,
                                {r.path, it->second.contents, 0, 0,
                                 it->second.contents.size(), it->second.uuid}};
    }
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

caas::CompileStatus do_compile(
    VFS *vfs,
    caas::MessagePump<caas::HostToCompiler, caas::CompilerToHost, Pipe> *pump,
    const std::vector<Source> &sources,
    const std::vector<std::string> &compile_opts,
    const std::vector<std::string> &link_opts, caas::CompileMode mode,
    uint32_t id = 0) {
  caas::CompileRequest req;
  req.cc_opts = compile_opts;
  req.ld_opts = link_opts;
  req.mode = mode;
  req.cwd = "/";
  req.output_binary = "/o/a.out";
  req.id = id;

  for (auto &s : sources) {
    req.srcs.emplace_back(s.filename, "/o" + s.filename + ".obj", true, true);
  }

  pump->Write(req);
  while (true) {
    pump->Pump();
    caas::CompilerToHost m;
    switch (pump->Read(m)) {
    case caas::ReadStatus::Ok:
      if (std::holds_alternative<caas::StatRequest>(m.msg)) {
        vfs->stat(std::get<caas::StatRequest>(m.msg));
      } else if (std::holds_alternative<caas::OpenRequest>(m.msg)) {
        vfs->open(std::get<caas::OpenRequest>(m.msg));
      } else if (std::holds_alternative<caas::CompileStatus>(m.msg)) {
        return std::get<caas::CompileStatus>(m.msg);
      }
      break;
    case caas::ReadStatus::Invalid:
      abort();
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
               std::vector<std::string> link_opts, caas::CompileMode mode) {
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
  auto status =
      do_compile(&vfs, test_host_pump, sources, compile_opts, link_opts, mode);
  return std::make_pair(status.error, status.output);
}

void test_compile1() {
  std::vector<std::string> compile_opts;
  std::vector<std::string> link_opts;
  std::vector<Source> sources;

  sources.emplace_back("/s/main.cc", R"(
int main() { return 0; }
)");

  auto err = do_compile_env(sources, compile_opts, link_opts,
                            caas::CompileMode::CompileAndLink);
}

void test_compile2() {
  std::vector<std::string> compile_opts;
  std::vector<std::string> link_opts;
  std::vector<Source> sources;

  sources.emplace_back("/s/main.cc", R"(
int main() { return "errored"; }
)");

  auto err = do_compile_env(sources, compile_opts, link_opts,
                            caas::CompileMode::CompileAndLink);
}

void test_compile3() {
  std::vector<std::string> compile_opts;
  std::vector<std::string> link_opts;
  std::vector<Source> sources;

  sources.emplace_back("/s/main.cc", R"(
int f() { return 0; }
)");

  auto err = do_compile_env(sources, compile_opts, link_opts,
                            caas::CompileMode::CompileAndLink);
}

void test_compile_thread() {
  // test_compile1();
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

  auto pump = std::make_unique<
      caas::MessagePump<caas::HostToCompiler, caas::CompilerToHost, Pipe>>(
      test_host_pipe.get());
  test_host_pump = pump.get();
  SetCompilerPipe(test_compiler_pipe.get());

  std::thread(test_compile_thread).detach();
}
