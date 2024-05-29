
#include "../../headers/graphics/GPU_handler.h"

void DX_Init()
{
    SCREEN->aspectRatio = (float)SCREEN->screenWidth / (float)SCREEN->screenHeight;
    SCREEN->frameIndex = 0;
    SCREEN->viewport = (D3D12_VIEWPORT){0.0f, 0.0f, (float)SCREEN->screenWidth, (float)SCREEN->screenHeight, 0.0f, 1.0f};
    SCREEN->scissorRect = (D3D12_RECT){0, 0, (LONG)SCREEN->screenWidth, (LONG)SCREEN->screenHeight};
    SCREEN->rtvDescriptorSize = 0;
    GetCurrentPath(SCREEN->assetsPath, _countof(SCREEN->assetsPath));

    LoadPipeline();
    LoadAssets();
}

void LoadPipeline()
{
    int isDebugFactory = 0;

#ifdef DEBUG

    ID3D12Debug *debugController = NULL;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        CALL(EnableDebugLayer, debugController);
        isDebugFactory |= DXGI_CREATE_FACTORY_DEBUG;
        RELEASE(debugController);
    }

#endif

    IDXGIFactory4 *factory = NULL;
    ExitIfFailed(CreateDXGIFactory2(isDebugFactory, IID_PPV_ARGS(&factory)));

    IDXGIAdapter1 *hardwareAdapter = NULL;
    IDXGIFactory1 *factoryAsFactory1 = NULL;

    ExitIfFailed(CAST(factory, factoryAsFactory1));

    GetHardwareAdapter(factoryAsFactory1, &hardwareAdapter, false);
}

void LoadAssets()
{
}
