
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

    // Create our factory
    IDXGIFactory4 *factory4 = NULL;
    HRESULT_EXIT_IF_FAILED(CreateDXGIFactory2(isDebugFactory, IID_PPV_ARGS(&factory4)));

    IDXGIAdapter1 *hardwareAdapter = NULL;

    // Find an adapter that has at least D3D11 support
    IDXGIFactory6 *factory6 = NULL;
    if (SUCCEEDED(CAST(factory4, factory6)))
    {
        DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_UNSPECIFIED;

        for (int i = 0;
             SUCCEEDED(CALL(EnumAdapterByGpuPreference,
                            factory6, i, gpuPreference, IID_PPV_ARGS(&hardwareAdapter)));
             i++)
        {
            DXGI_ADAPTER_DESC1 adapterDescription;
            CALL(GetDesc1, hardwareAdapter, &adapterDescription);
            if (adapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            IUnknown *hardwareAdapterAsIUnknown = NULL;
            HRESULT_EXIT_IF_FAILED(CAST(hardwareAdapter, hardwareAdapterAsIUnknown));
            if (SUCCEEDED(D3D12CreateDevice(hardwareAdapterAsIUnknown, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, NULL)))
            {
                RELEASE(hardwareAdapterAsIUnknown);
                break;
            }
        }
    }

    if (hardwareAdapter == NULL)
    {
        TCHAR buffer[64];
        _stprintf(buffer, TEXT("Unable to find adapter with D3D11 support."));
        OutputDebugString(buffer);
        exit(EXIT_FAILURE);
    }

    // Create device
    IUnknown *hardwareAdapterAsIUnknown = NULL;
    HRESULT_EXIT_IF_FAILED(CAST(hardwareAdapter, hardwareAdapterAsIUnknown));
    HRESULT_EXIT_IF_FAILED(D3D12CreateDevice(hardwareAdapterAsIUnknown, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&SCREEN->device)));
    RELEASE(hardwareAdapterAsIUnknown);
    RELEASE(hardwareAdapter);

    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {0};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HRESULT_EXIT_IF_FAILED(CALL(CreateCommandQueue, SCREEN->device, &queueDesc, IID_PPV_ARGS(&SCREEN->commandQueue)));

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = 1280; // TODO: Change these values to our screen size
    swapChainDesc.Height = 720;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    IUnknown *commandQueueAsIUnknown = NULL;
    HRESULT_EXIT_IF_FAILED(CAST(SCREEN->commandQueue, commandQueueAsIUnknown));
    IDXGISwapChain1 *swapChainAsSwapChain1 = NULL;
    HRESULT_EXIT_IF_FAILED(CALL(CreateSwapChainForHwnd,
                                factory4,
                                commandQueueAsIUnknown,
                                SCREEN->windowHandle,
                                &swapChainDesc,
                                NULL, // Fullscreen swap chain?
                                NULL,
                                &swapChainAsSwapChain1));
    RELEASE(commandQueueAsIUnknown);
    HRESULT_EXIT_IF_FAILED(CAST(swapChainAsSwapChain1, SCREEN->swapChain));
    RELEASE(swapChainAsSwapChain1);
    HRESULT_EXIT_IF_FAILED(CALL(MakeWindowAssociation, factory4, SCREEN->windowHandle, DXGI_MWA_NO_ALT_ENTER));
    SCREEN->frameIndex = CALL(GetCurrentBackBufferIndex, SCREEN->swapChain);

    // Create descripter heaps
}

void LoadAssets()
{
}
