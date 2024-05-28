
#include "../../headers/graphics/GPUHandler.h"

void DX_Init()
{
    SCREEN->aspectRatio = (float)SCREEN->screenWidth / (float)SCREEN->screenHeight;
    SCREEN->frameIndex = 0;
    SCREEN->viewport = (D3D12_VIEWPORT){0.0f, 0.0f, (float)SCREEN->screenWidth, (float)SCREEN->screenHeight, 0.0f, 1.0f};
    SCREEN->scissorRect = (D3D12_RECT){0, 0, (LONG)SCREEN->screenWidth, (LONG)SCREEN->screenHeight};
    SCREEN->rtvDescriptorSize = 0;
    GetCurrentPath(SCREEN->assetsPath, _countof(SCREEN->assetsPath));
}
