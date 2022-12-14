#pragma once

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/VirtualFileSystem.h"

using namespace llvm;
using namespace llvm::vfs;

vfs::FileSystem *CreateVFS(std::string cwd);
