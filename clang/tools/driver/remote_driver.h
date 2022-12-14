#pragma once

#include "caas_messages.h"
#include "util.h"

typedef caas::MessagePump<caas::CompilerToHost, caas::HostToCompiler, Pipe>
    CompilerPump;

extern caas::MessagePump<caas::CompilerToHost, caas::HostToCompiler, Pipe>
    *g_msgpump;
extern uint32_t g_compileID;

void test_compile();
