#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>
#include "dx12texturemanager.h"
#include "dx12renderpipeline.h"

class dx12command
{
private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_command_list;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fence_value;

	const UINT64 c_initial_value = 0;
public:
	dx12command(const D3D12_COMMAND_LIST_TYPE& command_type);
	~dx12command();
	void TransistionBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES before_state, D3D12_RESOURCE_STATES after_state);
	void ResourceBarrier(const D3D12_RESOURCE_BARRIER_TYPE& barrier_type, ID3D12Resource* resource);
	void CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION* destination, D3D12_TEXTURE_COPY_LOCATION* source);
	void CopyBufferRegion(ID3D12Resource* destination_buffer, ID3D12Resource* source_buffer, UINT destination_offset, UINT source_offset, UINT size);
	void CopyResource(ID3D12Resource* destination_resource, ID3D12Resource* source_resource);
	void Execute();
	void Signal();
	void Wait();
	void SignalAndWait();
	void SetDescriptorHeap(ID3D12DescriptorHeap* descriptor_heap);;
	void SetRootSignature(ID3D12RootSignature* root_signature);
	void SetComputeRootSignature(ID3D12RootSignature* root_signature);
	void SetPipelineState(ID3D12PipelineState* pipeline_state);
	void SetStateObject(ID3D12StateObject* state_object);
	void ClearRenderTargetView(ID3D12DescriptorHeap* descriptor_heap, UINT offset);
	void ClearDepthStencilView(ID3D12DescriptorHeap* descriptor_heap, UINT offset);
	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology);
	void SetOMRenderTargets(ID3D12DescriptorHeap* render_target_heap, UINT rtv_offset, ID3D12DescriptorHeap* depth_stencil_heap, UINT dsv_offset);
	void SetViewport(float width, float height);
	void SetScissorRect(float width, float height);
	void Reset();
	void SetShaderResourceView(RootRenderBinding* binding, ID3D12Resource* resource);
	void SetDescriptorTable(RootRenderBinding* binding, ID3D12DescriptorHeap* descriptor_heap, dx12texture& resource);
	void SetComputeDescriptorTable(RootRenderBinding* binding, ID3D12DescriptorHeap* descriptor_heap, dx12texture& resource);
	void Draw(UINT vertices, UINT nr_of_objects, UINT start_vertex, UINT start_object);
	void DispatchRays(D3D12_DISPATCH_RAYS_DESC* description);
	void BuildRaytracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc);
	void EndQuery(ID3D12QueryHeap* query, const D3D12_QUERY_TYPE& query_type, UINT index);
	void ResolveQueryData(ID3D12QueryHeap* query, const D3D12_QUERY_TYPE& query_type, UINT index, UINT num_queries, ID3D12Resource* resource, UINT64 offset);

	ID3D12GraphicsCommandList4* GetCommandList();
};

