
#include "GPU_handler.h"

void Directx_Init()
{
    SCREEN->aspectRatio = (float)SCREEN->screenWidth / (float)SCREEN->screenHeight;
    SCREEN->frameIndex = 0;
    SCREEN->viewport = (D3D12_VIEWPORT){0.0f, 0.0f, (float)SCREEN->screenWidth, (float)SCREEN->screenHeight, 0.0f, 1.0f};
    SCREEN->scissorRect = (D3D12_RECT){0, 0, (LONG)SCREEN->screenWidth, (LONG)SCREEN->screenHeight};
    SCREEN->rtvDescriptorSize = 0;

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
    HANDLE_HRESULT(CreateDXGIFactory2(isDebugFactory, IID_PPV_ARGS(&factory4)));

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
            HANDLE_HRESULT(CAST(hardwareAdapter, hardwareAdapterAsIUnknown));
            if (SUCCEEDED(D3D12CreateDevice(hardwareAdapterAsIUnknown, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, NULL)))
            {
                RELEASE(hardwareAdapterAsIUnknown);
                break;
            }
        }
        RELEASE(factory6);
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
    HANDLE_HRESULT(CAST(hardwareAdapter, hardwareAdapterAsIUnknown));
    HANDLE_HRESULT(D3D12CreateDevice(hardwareAdapterAsIUnknown, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&SCREEN->device)));
    RELEASE(hardwareAdapterAsIUnknown);
    RELEASE(hardwareAdapter);

    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {0};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HANDLE_HRESULT(CALL(CreateCommandQueue, SCREEN->device, &queueDesc, IID_PPV_ARGS(&SCREEN->commandQueue)));

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.Width = SCREEN->screenWidth;
    swapChainDesc.Height = SCREEN->screenHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    IUnknown *commandQueueAsIUnknown = NULL;
    HANDLE_HRESULT(CAST(SCREEN->commandQueue, commandQueueAsIUnknown));
    IDXGISwapChain1 *swapChainAsSwapChain1 = NULL;
    HANDLE_HRESULT(CALL(CreateSwapChainForHwnd,
                        factory4,
                        commandQueueAsIUnknown,
                        SCREEN->windowHandle,
                        &swapChainDesc,
                        NULL, // Fullscreen swap chain?
                        NULL,
                        &swapChainAsSwapChain1));
    RELEASE(commandQueueAsIUnknown);
    HANDLE_HRESULT(CAST(swapChainAsSwapChain1, SCREEN->swapChain));
    RELEASE(swapChainAsSwapChain1);
    HANDLE_HRESULT(CALL(MakeWindowAssociation, factory4, SCREEN->windowHandle, DXGI_MWA_NO_ALT_ENTER));
    SCREEN->frameIndex = CALL(GetCurrentBackBufferIndex, SCREEN->swapChain);

    // Create descripter heaps
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {0};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = FRAME_COUNT;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HANDLE_HRESULT(CALL(CreateDescriptorHeap, SCREEN->device, &rtvHeapDesc, IID_PPV_ARGS(&SCREEN->rtvHeap)));
        SCREEN->rtvDescriptorSize = CALL(GetDescriptorHandleIncrementSize, SCREEN->device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources on the descriptor heaps
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
        CALL(GetCPUDescriptorHandleForHeapStart, SCREEN->rtvHeap, &rtvHandle);

        for (unsigned int i = 0; i < FRAME_COUNT; i++)
        {
            // As a note, this function call states you cannot access buffers past the first with the swap chain swap effect
            // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-getbuffer
            HANDLE_HRESULT(CALL(GetBuffer, SCREEN->swapChain, i, IID_PPV_ARGS(&SCREEN->renderTargets[i])));
            CALL(CreateRenderTargetView, SCREEN->device, SCREEN->renderTargets[i], NULL, rtvHandle);
            rtvHandle.ptr = (SIZE_T)((INT64)(rtvHandle.ptr) + (INT64)(SCREEN->rtvDescriptorSize));
        }
    }

    HANDLE_HRESULT(CALL(CreateCommandAllocator, SCREEN->device, D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&SCREEN->commandAllocator)));
    RELEASE(factory4);
}

void LoadAssets()
{
    // Create root signature
    {
        const D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {
            .NumParameters = 0,
            .pParameters = NULL,
            .NumStaticSamplers = 0,
            .pStaticSamplers = NULL,
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};

        ID3DBlob *signature = NULL;
        ID3DBlob *error = NULL;
        HANDLE_HRESULT(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        const LPVOID bufferPointer = CALL(GetBufferPointer, signature);
        const SIZE_T bufferSize = CALL(GetBufferSize, signature);
        HANDLE_HRESULT(CALL(CreateRootSignature, SCREEN->device, 0, bufferPointer, bufferSize, IID_PPV_ARGS(&SCREEN->rootSignature)));

        // Allegedly ID3DBlob is not reference counted and do not need to be released but just in case.
        RELEASE(signature);
        RELEASE(error);
    }

    // Create the pipeline state, which includes compiling and loading shaders
    {
        ID3DBlob *vertexShader = NULL;
        ID3DBlob *pixelShader = NULL;

#ifdef DEBUG
        // Enable better shader debugging with the graphics debugging tools.
        const UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        const UINT compileFlags = 0;
#endif

        HANDLE_HRESULT(D3DCompile(shaderCode, sizeof(shaderCode), NULL, NULL, NULL, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, NULL));
        HANDLE_HRESULT(D3DCompile(shaderCode, sizeof(shaderCode), NULL, NULL, NULL, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, NULL));

        // Define the vertex input layout
        const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                {.SemanticName = "POSITION", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0, .AlignedByteOffset = 0, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0},
                {.SemanticName = "COLOR", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .InputSlot = 0, .AlignedByteOffset = 12, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0}};

        // Create PSO

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {0};
        psoDesc.pRootSignature = SCREEN->rootSignature;
        psoDesc.VS = (D3D12_SHADER_BYTECODE){
            .pShaderBytecode = CALL(GetBufferPointer, vertexShader),
            .BytecodeLength = CALL(GetBufferSize, vertexShader),
        };
        psoDesc.PS = (D3D12_SHADER_BYTECODE){
            .pShaderBytecode = CALL(GetBufferPointer, pixelShader),
            .BytecodeLength = CALL(GetBufferSize, pixelShader),
        };

        D3D12_BLEND_DESC blendDesc;
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        {
            blendDesc.RenderTarget[i] = (D3D12_RENDER_TARGET_BLEND_DESC){
                .BlendEnable = FALSE,
                .LogicOpEnable = FALSE,
                .SrcBlend = D3D12_BLEND_ONE,
                .DestBlend = D3D12_BLEND_ZERO,
                .BlendOp = D3D12_BLEND_OP_ADD,
                .SrcBlendAlpha = D3D12_BLEND_ONE,
                .DestBlendAlpha = D3D12_BLEND_ZERO,
                .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                .LogicOp = D3D12_LOGIC_OP_NOOP,
                .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
            };
        }

        psoDesc.BlendState = blendDesc;

        psoDesc.SampleMask = UINT_MAX;

        D3D12_RASTERIZER_DESC rasterizerDesc = {
            .FillMode = D3D12_FILL_MODE_WIREFRAME,
            .CullMode = D3D12_CULL_MODE_BACK,
            .FrontCounterClockwise = FALSE,
            .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
            .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
            .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
            .DepthClipEnable = TRUE,
            .MultisampleEnable = FALSE,
            .AntialiasedLineEnable = TRUE,
            .ForcedSampleCount = 16,
            .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
        };

        psoDesc.RasterizerState = rasterizerDesc;

        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;

        psoDesc.InputLayout = (D3D12_INPUT_LAYOUT_DESC){
            .pInputElementDescs = inputElementDescs,
            .NumElements = _countof(inputElementDescs)};

        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        HANDLE_HRESULT(CALL(CreateGraphicsPipelineState, SCREEN->device, &psoDesc, IID_PPV_ARGS(&SCREEN->pipelineState)));
    }

    // Create the command list

    HANDLE_HRESULT(CALL(CreateCommandList, SCREEN->device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, SCREEN->commandAllocator, SCREEN->pipelineState, IID_PPV_ARGS(&SCREEN->commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now
    HANDLE_HRESULT(CALL(Close, SCREEN->commandList));

    // Create synchronization objects and wait until assets have been uploaded to the GPU
    {
        HANDLE_HRESULT(CALL(CreateFence, SCREEN->device, 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&SCREEN->fence)));
        SCREEN->fenceValue = 1;

        // Create an event handle to use for frame synchronization
        HANDLE_ERROR((SCREEN->fenceEvent = CreateEvent(NULL, TRUE, FALSE, NULL)));

        // Wait for the command list to execute; we are reusing the same command
        // list in our main loop, but for now, we just want to wait for setup to
        // complete before continuing
        Directx_SetFenceAndWait();
    }
}

void Directx_SetupRender()
{
    if (WaitForSingleObject(SCREEN->fenceEvent, 0) != WAIT_OBJECT_0)
    {
        return;
    }

    if (WaitForSingleObject(SCREEN->preRenderSetupMutex, 0) != WAIT_OBJECT_0)
    {
        return;
    }

    ResetEvent(SCREEN->fenceEvent);

    SCREEN->frameIndex = CALL(GetCurrentBackBufferIndex, SCREEN->swapChain);

    PopulateCommandList();

    ID3D12CommandList *asCommandList = NULL;
    CAST(SCREEN->commandList, asCommandList);
    ID3D12CommandList *ppCommandLists[] = {asCommandList};
    CALL(ExecuteCommandLists, SCREEN->commandQueue, _countof(ppCommandLists), ppCommandLists);
    RELEASE(asCommandList);

    HANDLE_HRESULT(CALL(Present, SCREEN->swapChain, 1, 0));

    Directx_SetFence();
    ReleaseMutex(SCREEN->preRenderSetupMutex);
}

// Make sure to reset the fence
void Directx_SetFence()
{
    HANDLE_HRESULT(CALL(Signal, SCREEN->commandQueue, SCREEN->fence, SCREEN->fenceValue));

    if (CALL(GetCompletedValue, SCREEN->fence) < SCREEN->fenceValue)
    {
        HANDLE_HRESULT(CALL(SetEventOnCompletion, SCREEN->fence, SCREEN->fenceValue, SCREEN->fenceEvent));
    }
    else
    {
        SetEvent(SCREEN->fenceEvent);
    }

    SCREEN->fenceValue++;
}

void Directx_SetFenceAndWait()
{
    ResetEvent(SCREEN->fenceEvent);
    Directx_SetFence();

    WaitForSingleObject(SCREEN->fenceEvent, INFINITE);
}

void PopulateCommandList()
{
    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU; apps should use
    // fences to determine GPU execution progress
    HANDLE_HRESULT(CALL(Reset, SCREEN->commandAllocator));

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording
    HANDLE_HRESULT(CALL(Reset, SCREEN->commandList, SCREEN->commandAllocator, SCREEN->pipelineState));

    // Set necessary state
    CALL(SetGraphicsRootSignature, SCREEN->commandList, SCREEN->rootSignature);
    CALL(RSSetViewports, SCREEN->commandList, 1, &SCREEN->viewport);
    CALL(RSSetScissorRects, SCREEN->commandList, 1, &SCREEN->scissorRect);

    // Indicate that the back buffer will be used as a render target
    const D3D12_RESOURCE_BARRIER transitionBarrierRT = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = SCREEN->renderTargets[SCREEN->frameIndex],
            .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
            .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        }};

    CALL(ResourceBarrier, SCREEN->commandList, 1, &transitionBarrierRT);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    CALL(GetCPUDescriptorHandleForHeapStart, SCREEN->rtvHeap, &rtvHandle);
    const INT64 CurrentRtvOffset = SCREEN->frameIndex * SCREEN->rtvDescriptorSize;
    rtvHandle.ptr = (SIZE_T)((INT64)(rtvHandle.ptr) + CurrentRtvOffset);
    CALL(OMSetRenderTargets, SCREEN->commandList, 1, &rtvHandle, FALSE, NULL);

    // Record commands
    const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    CALL(ClearRenderTargetView, SCREEN->commandList, rtvHandle, clearColor, 0, NULL);
    CALL(IASetPrimitiveTopology, SCREEN->commandList, D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);

    if (SCREEN->screenEntity != NULL)
    {
        Point *screenEntityLocation;
        ReadWriteLock_GetReadPermission(&((Entity *)SCREEN->screenEntity)->location, (void **)&screenEntityLocation);

        SCREEN->screenLocation.x = screenEntityLocation->x;
        SCREEN->screenLocation.y = screenEntityLocation->y;

        ReadWriteLock_ReleaseReadPermission(&((Entity *)SCREEN->screenEntity)->location, (void **)&screenEntityLocation);
    }

    // TODO: This should be parallelized via the thread list iterator and task system
    // I'm going to wait for internal memory management to be set up and Queue to be added
    // The above doesn't look great either, the gamestate is being updated while we
    // process the command lists which is changing the location of the object awkwardly
    // It's either we have a jittery screen or jittery other entities for now but we can
    // (mostly) fix this once we parallelize the command list processing as it takes precedence
    // over the gamestate processing

    List *entities;
    ReadWriteLockPriority_GetPriorityReadPermission(&GAMESTATE->entities, (void **)&entities);

    ListIterator entitiesIterator;
    ListIterator_Init(&entitiesIterator, entities);

    Entity *entity;
    while (ListIterator_Next(&entitiesIterator, (void **)&entity))
    {
        if (entity->alive == ENTITY_DEAD)
        {
            continue;
        }

        ListIterator onRenderIterator;
        ListIterator_Init(&onRenderIterator, &entity->onRender);
        void (*onRender)(Entity *);
        while (ListIterator_Next(&onRenderIterator, (void **)&onRender))
        {
            onRender(entity);
        }
    }

    ReadWriteLockPriority_ReleaseReadPermission(&GAMESTATE->entities, (void **)&entities);

    D3D12_RESOURCE_BARRIER transitionBarrierPresent = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
            .pResource = SCREEN->renderTargets[SCREEN->frameIndex],
            .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
            .StateAfter = D3D12_RESOURCE_STATE_PRESENT,
            .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        }};

    // Indicate that the back buffer will now be used to present
    CALL(ResourceBarrier, SCREEN->commandList, 1, &transitionBarrierPresent);
    HANDLE_HRESULT(CALL(Close, SCREEN->commandList));
}

void ReleaseDirectxObjects()
{
    RELEASE(SCREEN->swapChain);
    RELEASE(SCREEN->device);
    for (int i = 0; i < FRAME_COUNT; ++i)
    {
        RELEASE(SCREEN->renderTargets[i]);
    }
    RELEASE(SCREEN->commandAllocator);
    RELEASE(SCREEN->commandQueue);
    RELEASE(SCREEN->rootSignature);
    RELEASE(SCREEN->rtvHeap);
    RELEASE(SCREEN->pipelineState);
    RELEASE(SCREEN->commandList);
    RELEASE(SCREEN->fence);
}
