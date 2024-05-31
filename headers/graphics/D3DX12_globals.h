#pragma once
#ifndef D3DX12_MACROS_H_
#define D3DX12_MACROS_H_

#include <d3d12.h>
#include <dxgi1_6.h>

typedef struct IDXGIFactory1 IDXGIFactory1;
typedef struct IDXGIAdapter1 IDXGIAdapter1;
typedef struct IDXGISwapChain3 IDXGISwapChain3;

#define __extractIID(X) _Generic((X),                              \
    ID3D12Device * *: &IID_ID3D12Device,                           \
    ID3D12Debug * *: &IID_ID3D12Debug,                             \
    IDXGIAdapter1 * *: &IID_IDXGIAdapter1,                         \
    IDXGIFactory1 * *: &IID_IDXGIFactory1,                         \
    IDXGIFactory4 * *: &IID_IDXGIFactory4,                         \
    IDXGIFactory6 * *: &IID_IDXGIFactory6,                         \
    ID3D12CommandQueue * *: &IID_ID3D12CommandQueue,               \
    ID3D12CommandList * *: &IID_ID3D12CommandList,                 \
    ID3D12Resource * *: &IID_ID3D12Resource,                       \
    ID3D12RootSignature * *: &IID_ID3D12RootSignature,             \
    ID3D12PipelineState * *: &IID_ID3D12PipelineState,             \
    ID3D12GraphicsCommandList * *: &IID_ID3D12GraphicsCommandList, \
    ID3D12DescriptorHeap * *: &IID_ID3D12DescriptorHeap,           \
    ID3D12CommandAllocator * *: &IID_ID3D12CommandAllocator,       \
    IDXGISwapChain1 * *: &IID_IDXGISwapChain1,                     \
    IDXGISwapChain3 * *: &IID_IDXGISwapChain3,                     \
    ID3DBlob * *: &IID_ID3DBlob,                                   \
    IUnknown * *: &IID_IUnknown,                                   \
    ID3D12Fence * *: &IID_ID3D12Fence)

#define IID_PPV_ARGS(ppType) __extractIID(ppType), (void **)(ppType)

// The ## __VA_ARGS__ is not portable, being a GCC extension https://gcc.gnu.org/onlinedocs/gcc/Variadic-Macros.html
// But apparently, it also works in MSVC as well
#define CALL(METHOD, CALLER, ...) CALLER->lpVtbl->METHOD(CALLER, ##__VA_ARGS__)

// cast COM style
#define CAST(from, to) CALL(QueryInterface, from, IID_PPV_ARGS(&to))

// Calls Release and makes the pointer NULL if RefCount reaches zero
#define RELEASE(ptr)                           \
    if (ptr && ptr->lpVtbl->Release(ptr) == 0) \
    ptr = NULL

typedef struct float3
{
    float x;
    float y;
    float z;
} float3;

typedef struct float4
{
    float x;
    float y;
    float z;
    float w;
} float4;

typedef struct Vertex
{
    float3 position;
    float4 color;
} Vertex;

#endif
