#pragma once
#ifndef GPU_HANDLER_H_
#define GPU_HANDLER_H_

#include "../globals.h"
#include "../entity.h"

void Directx_Init();
void LoadPipeline();
void LoadAssets();

void Directx_SetupRender();

void Directx_SetFence();
void Directx_SetFenceAndWait();
void PopulateCommandList();

void ReleaseDirectxObjects();

#endif
