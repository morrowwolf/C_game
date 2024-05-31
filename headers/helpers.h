#pragma once
#ifndef HELPERS_H_
#define HELPERS_H_

#include <Windows.h>

void GetCurrentPath(_Out_writes_(pathSize) WCHAR *const path, UINT pathSize);

#endif
