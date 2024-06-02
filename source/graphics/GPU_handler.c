
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
            if (SUCCEEDED(D3D12CreateDevice(hardwareAdapterAsIUnknown, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, NULL)))
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
    HANDLE_HRESULT(D3D12CreateDevice(hardwareAdapterAsIUnknown, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&SCREEN->device)));
    RELEASE(hardwareAdapterAsIUnknown);
    RELEASE(hardwareAdapter);

    // Create command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {0};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HANDLE_HRESULT(CALL(CreateCommandQueue, SCREEN->device, &queueDesc, IID_PPV_ARGS(&SCREEN->commandQueue)));

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.BufferCount = 2; // Pretty sure this should be FRAME_COUNT
    swapChainDesc.Width = 1280;    // TODO: Change these values to our screen size
    swapChainDesc.Height = 720;
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

        const wchar_t *shadersPath = wcscat(SCREEN->assetsPath, L"shaders/shaders.hlsl");
        HANDLE_HRESULT(D3DCompileFromFile(shadersPath, NULL, NULL, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, NULL));
        HANDLE_HRESULT(D3DCompileFromFile(shadersPath, NULL, NULL, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, NULL));

        // Define the vertex input layout
        const D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                {.SemanticName = "POSITION", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32_FLOAT, .InputSlot = 0, .AlignedByteOffset = 0, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0},
                {.SemanticName = "COLOR", .SemanticIndex = 0, .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .InputSlot = 0, .AlignedByteOffset = 12, .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, .InstanceDataStepRate = 0}};

        // Create PSO

        // STOPPED HERE

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {0};
        psoDesc.pRootSignature = SCREEN->rootSignature;
        psoDesc.InputLayout = (D3D12_INPUT_LAYOUT_DESC){
            .pInputElementDescs = inputElementDescs,
            .NumElements = _countof(inputElementDescs)};
        psoDesc.VS = (D3D12_SHADER_BYTECODE){
            .pShaderBytecode = CALL(GetBufferPointer, vertexShader),
            .BytecodeLength = CALL(GetBufferSize, vertexShader),
        };
        psoDesc.PS = (D3D12_SHADER_BYTECODE){
            .pShaderBytecode = CALL(GetBufferPointer, pixelShader),
            .BytecodeLength = CALL(GetBufferSize, pixelShader),
        };
        psoDesc.RasterizerState = CD3DX12_DEFAULT_RASTERIZER_DESC();
        psoDesc.BlendState = CD3DX12_DEFAULT_BLEND_DESC();
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SCREENMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SCREENDesc.Count = 1;
        HANDLE_HRESULT(CALL(CreateGraphicsPipelineState, SCREEN->device, &psoDesc, IID_PPV_ARGS(&SCREEN->pipelineState)));
    }

    /* Create the command list */

    HANDLE_HRESULT(CALL(CreateCommandList, SCREEN->device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, SCREEN->commandAllocator, SCREEN->pipelineState, IID_PPV_ARGS(&SCREEN->commandList)));
    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now
    HANDLE_HRESULT(CALL(Close, SCREEN->commandList));

    /* Create the vertex buffer, populate it and set a view to it */
    {
        // Coordinates are in relation to the screen center, left-handed (+z to screen inside, +y up, +x right)
        const Vertex triangleVertices[] =
            {
                {{0.0f, 0.25f * SCREEN->aspectRatio, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {{0.25f, -0.25f * SCREEN->aspectRatio, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                {{-0.25f, -0.25f * SCREEN->aspectRatio, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            };
        const UINT vertexBufferSize = sizeof(triangleVertices);
        const D3D12_HEAP_PROPERTIES heapPropertyUpload = (D3D12_HEAP_PROPERTIES){
            .Type = D3D12_HEAP_TYPE_UPLOAD,
            .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
            .CreationNodeMask = 1,
            .VisibleNodeMask = 1,
        };
        const D3D12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC_BUFFER(vertexBufferSize, D3D12_RESOURCE_FLAG_NONE, 0);

        // Note: using upload heaps to transfer static data like vert buffers is not
        // recommended. Every time the GPU needs it, the upload heap will be marshalled
        // over. Please read up on Default Heap usage. An upload heap is used here for
        // code simplicity and because there are very few verts to actually transfer
        HANDLE_HRESULT(CALL(CreateCommittedResource, SCREEN->device,
                            &heapPropertyUpload,
                            D3D12_HEAP_FLAG_NONE,
                            &bufferResource,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            NULL,
                            IID_PPV_ARGS(&SCREEN->vertexBuffer)));

        // We will open the vertexBuffer memory (that is in the GPU) for the CPU to write the triangle data
        // in it. To do that, we use the Map() function, which enables the CPU to read from or write
        // to the vertex buffer's memory directly
        UINT8 *pVertexDataBegin = NULL; // UINT8 to represent byte-level manipulation
        // We do not intend to read from this resource on the CPU, only write
        const D3D12_RANGE readRange = (D3D12_RANGE){.Begin = 0, .End = 0};
        HANDLE_HRESULT(CALL(Map, SCREEN->vertexBuffer, 0, &readRange, (void **)(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        // While mapped, the GPU cannot access the buffer, so it's important to minimize the time
        // the buffer is mapped.
        CALL(Unmap, SCREEN->vertexBuffer, 0, NULL);

        // Initialize the vertex buffer view
        SCREEN->vertexBufferView.BufferLocation = CALL(GetGPUVirtualAddress, SCREEN->vertexBuffer);
        SCREEN->vertexBufferView.StrideInBytes = sizeof(Vertex);
        SCREEN->vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU
    {
        HANDLE_HRESULT(CALL(CreateFence, SCREEN->device, 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&SCREEN->fence)));
        SCREEN->fenceValue = 1;

        // Create an event handle to use for frame synchronization
        HANDLE_ERROR(SCREEN->fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL));

        // Wait for the command list to execute; we are reusing the same command
        // list in our main loop, but for now, we just want to wait for setup to
        // complete before continuing
        WaitForPreviousFrame();
    }
}

void WaitForPreviousFrame()
{
    // Signal to the fence the current fenceValue
    HANDLE_HRESULT(CALL(Signal, SCREEN->commandQueue, SCREEN->fence, SCREEN->fenceValue));
    // Wait until the frame is finished (ie. reached the signal sent right above)
    if (CALL(GetCompletedValue, SCREEN->fence) < SCREEN->fenceValue)
    {
        HANDLE_HRESULT(CALL(SetEventOnCompletion, SCREEN->fence, SCREEN->fenceValue, SCREEN->fenceEvent));
        WaitForSingleObject(SCREEN->fenceEvent, INFINITE);
    }
    SCREEN->fenceValue++;
    SCREEN->frameIndex = CALL(GetCurrentBackBufferIndex, SCREEN->swapChain);
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
    const D3D12_RESOURCE_BARRIER transitionBarrierRT = CD3DX12_Transition(SCREEN->renderTargets[SCREEN->frameIndex],
                                                                          D3D12_RESOURCE_STATE_PRESENT,
                                                                          D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                          D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                                          D3D12_RESOURCE_BARRIER_FLAG_NONE);
    CALL(ResourceBarrier, SCREEN->commandList, 1, &transitionBarrierRT);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
    CALL(GetCPUDescriptorHandleForHeapStart, SCREEN->rtvHeap, &rtvHandle);
    const INT64 CurrentRtvOffset = SCREEN->frameIndex * SCREEN->rtvDescriptorSize;
    rtvHandle.ptr = (SIZE_T)((INT64)(rtvHandle.ptr) + CurrentRtvOffset);
    CALL(OMSetRenderTargets, SCREEN->commandList, 1, &rtvHandle, FALSE, NULL);

    // Record commands
    const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
    CALL(ClearRenderTargetView, SCREEN->commandList, rtvHandle, clearColor, 0, NULL);
    CALL(IASetVertexBuffers, SCREEN->commandList, 0, 1, &SCREEN->vertexBufferView);
    CALL(IASetPrimitiveTopology, SCREEN->commandList, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CALL(DrawInstanced, SCREEN->commandList, 3, 1, 0, 0);

    D3D12_RESOURCE_BARRIER transitionBarrierPresent = CD3DX12_Transition(SCREEN->renderTargets[SCREEN->frameIndex],
                                                                         D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                         D3D12_RESOURCE_STATE_PRESENT,
                                                                         D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                                                         D3D12_RESOURCE_BARRIER_FLAG_NONE);

    // Indicate that the back buffer will now be used to present
    CALL(ResourceBarrier, SCREEN->commandList, 1, &transitionBarrierPresent);
    HANDLE_HRESULT(CALL(Close, SCREEN->commandList));
}

void ReleaseAll()
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
    RELEASE(SCREEN->vertexBuffer);
    RELEASE(SCREEN->fence);
}
