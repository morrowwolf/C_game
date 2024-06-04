#pragma once
#ifndef GPU_HANDLER_H_
#define GPU_HANDLER_H_

#include "../globals.h"
#include "D3DX12_globals.h"

void Directx_Init();
void LoadPipeline();
void LoadAssets();

void Directx_Update();
void Directx_Render();

void WaitForPreviousFrame();
void PopulateCommandList();

void ReleaseDirectxObjects();

#endif
