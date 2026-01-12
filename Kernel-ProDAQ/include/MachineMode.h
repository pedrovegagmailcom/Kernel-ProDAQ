#pragma once

#include <stdint.h>

enum class MoveSense : uint8_t { Unknown = 0, Traction = 1, Compression = 2 };

extern volatile MoveSense g_lastMoveSense;
extern volatile bool g_modoCompresometro;
