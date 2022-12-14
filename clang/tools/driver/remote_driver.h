#pragma once

#include "flatbuffer_messages.h"

extern FlatbufferMessagePump<compiler::HostMessage> *g_pump;
extern uint32_t g_compileID;

void test_compile();
