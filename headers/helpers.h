#pragma once
#ifndef HELPERS_H_
#define HELPERS_H_

#define UNICODE
#define _UNICODE

#include <Windows.h>
#include <tchar.h>

void ExitIfFailed(const HRESULT);

void GetCurrentPath(_Out_writes_(pathSize) WCHAR *const path, UINT pathSize);

#endif
