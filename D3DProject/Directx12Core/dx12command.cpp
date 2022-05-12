#include "dx12command.h"
#include "dx12core.h"

dx12command::dx12command(const D3D12_COMMAND_LIST_TYPE& command_type)
{
	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateCommandAllocator(command_type, IID_PPV_ARGS(m_command_allocator.GetAddressOf()));
	assert(SUCCEEDED(hr));

	ID3D12GraphicsCommandList* temp_list;
	hr = dx12core::GetDx12Core().GetDevice()->CreateCommandList(0, command_type, m_command_allocator.Get(),
		nullptr, IID_PPV_ARGS(&temp_list));
	assert(SUCCEEDED(hr));

	hr = temp_list->QueryInterface(__uuidof(ID3D12GraphicsCommandList4),
		reinterpret_cast<void**>(m_command_list.GetAddressOf()));
	assert(SUCCEEDED(hr));

	hr = dx12core::GetDx12Core().GetDevice()->CreateFence(c_initial_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));
	assert(SUCCEEDED(hr));

	m_fence_value = c_initial_value;

	temp_list->Release();
}

dx12command::~dx12command()
{
}

void dx12command::TransistionBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES before_state, D3D12_RESOURCE_STATES after_state)
{
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before_state;
	barrier.Transition.StateAfter = after_state;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_command_list->ResourceBarrier(1, &barrier);
}

void dx12command::ResourceBarrier(const D3D12_RESOURCE_BARRIER_TYPE& barrier_type, ID3D12Resource* resource)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = barrier_type;

	switch (barrier_type)
	{
	case D3D12_RESOURCE_BARRIER_TYPE_UAV:
		barrier.UAV.pResource = resource;
		break;
	}
	m_command_list->ResourceBarrier(1, &barrier);
}

void dx12command::CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION* destination, D3D12_TEXTURE_COPY_LOCATION* source)
{
	m_command_list->CopyTextureRegion(destination, 0, 0, 0, source, nullptr);
}

void dx12command::CopyBufferRegion(ID3D12Resource* destination_buffer, ID3D12Resource* source_buffer, UINT destination_offset, UINT source_offset, UINT size)
{
	m_command_list->CopyBufferRegion(destination_buffer, destination_offset, source_buffer, source_offset, size);
}

void dx12command::CopyResource(ID3D12Resource* destination_resource, ID3D12Resource* source_resource)
{
	m_command_list->CopyResource(destination_resource, source_resource);
}

void dx12command::Execute()
{
	HRESULT hr = m_command_list->Close();
	assert(SUCCEEDED(hr));
	ID3D12CommandList* temp = m_command_list.Get();
	dx12core::GetDx12Core().GetCommandQueue()->ExecuteCommandLists(1, &temp);
}

void dx12command::Signal()
{
	++m_fence_value;
	HRESULT hr = dx12core::GetDx12Core().GetCommandQueue()->Signal(m_fence.Get(), m_fence_value);
	assert(SUCCEEDED(hr));
}

void dx12command::Wait()
{
	if (m_fence->GetCompletedValue() < m_fence_value)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, 0, 0, EVENT_ALL_ACCESS);
		HRESULT hr = m_fence->SetEventOnCompletion(m_fence_value, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void dx12command::SignalAndWait()
{
	Signal();
	Wait();

	dx12core::GetDx12Core().GetBufferManager()->ResetUploadBuffer();
}

void dx12command::SetDescriptorHeap(ID3D12DescriptorHeap* descriptor_heap)
{
	m_command_list->SetDescriptorHeaps(1, &descriptor_heap);
}

void dx12command::SetRootSignature(ID3D12RootSignature* root_signature)
{
	m_command_list->SetGraphicsRootSignature(root_signature);
}

void dx12command::SetComputeRootSignature(ID3D12RootSignature* root_signature)
{
	m_command_list->SetComputeRootSignature(root_signature);
}

void dx12command::SetPipelineState(ID3D12PipelineState* pipeline_state)
{
	m_command_list->SetPipelineState(pipeline_state);
}

void dx12command::SetStateObject(ID3D12StateObject* state_object)
{
	m_command_list->SetPipelineState1(state_object);
}

void dx12command::ClearRenderTargetView(ID3D12DescriptorHeap* descriptor_heap, UINT offset)
{
	float clearColour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT size = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	rtv_handle.ptr += size * offset;
	m_command_list->ClearRenderTargetView(rtv_handle, clearColour, 0, nullptr);
}

void dx12command::ClearDepthStencilView(ID3D12DescriptorHeap* descriptor_heap, UINT offset)
{
	UINT size = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	dsv_handle.ptr += size * offset;
	m_command_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void dx12command::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
	m_command_list->IASetPrimitiveTopology(topology);
}

void dx12command::SetOMRenderTargets(ID3D12DescriptorHeap* render_target_heap, UINT rtv_offset, ID3D12DescriptorHeap* depth_stencil_heap, UINT dsv_offset)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = render_target_heap->GetCPUDescriptorHandleForHeapStart();
	rtv_handle.ptr += dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * rtv_offset;

	D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = depth_stencil_heap->GetCPUDescriptorHandleForHeapStart();
	dsv_handle.ptr += dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV) * dsv_offset;

	m_command_list->OMSetRenderTargets(1, &rtv_handle, true, &dsv_handle);
}

void dx12command::SetViewport(float width, float height)
{
	D3D12_VIEWPORT viewport = { 0, 0, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
	m_command_list->RSSetViewports(1, &viewport);
}

void dx12command::SetScissorRect(float width, float height)
{
	D3D12_RECT scissorRect = { 0, 0, static_cast<long>(width), static_cast<long>(height) };
	m_command_list->RSSetScissorRects(1, &scissorRect);
}

void dx12command::Reset()
{
	HRESULT hr = m_command_allocator->Reset();
	assert(SUCCEEDED(hr));
	hr = m_command_list->Reset(m_command_allocator.Get(), nullptr); // nullptr is initial state, no initial state
	assert(SUCCEEDED(hr));
}

void dx12command::SetShaderResourceView(RootRenderBinding* binding, ID3D12Resource* resource)
{
	m_command_list->SetGraphicsRootShaderResourceView(binding->root_parameter_index, resource->GetGPUVirtualAddress());
}

void dx12command::SetDescriptorTable(RootRenderBinding* binding, ID3D12DescriptorHeap* descriptor_heap, dx12texture& resource)
{
	//resource.;
	UINT size = 0;

	if (binding->binding_type == BindingType::SHADER_RESOURCE || binding->binding_type == BindingType::STRUCTURED_BUFFER || binding->binding_type == BindingType::UNORDERED_ACCESS)
		size = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//Get the descriptor handle and the offset to move the pointer to the correct resource
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = descriptor_heap->GetGPUDescriptorHandleForHeapStart();
	gpu_handle.ptr += size * resource.descriptor_heap_offset; 

	m_command_list->SetGraphicsRootDescriptorTable(binding->root_parameter_index, gpu_handle);
}

void dx12command::SetComputeDescriptorTable(RootRenderBinding* binding, ID3D12DescriptorHeap* descriptor_heap, dx12texture& resource)
{
	UINT size = 0;

	if (binding->binding_type == BindingType::SHADER_RESOURCE || binding->binding_type == BindingType::STRUCTURED_BUFFER || binding->binding_type == BindingType::UNORDERED_ACCESS)
		size = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//Get the descriptor handle and the offset to move the pointer to the correct resource
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = descriptor_heap->GetGPUDescriptorHandleForHeapStart();
	gpu_handle.ptr += size * resource.descriptor_heap_offset;

	m_command_list->SetComputeRootDescriptorTable(binding->root_parameter_index, gpu_handle);
}

void dx12command::Draw(UINT vertices, UINT nr_of_objects, UINT start_vertex, UINT start_object)
{
	m_command_list->DrawInstanced(vertices, nr_of_objects, start_vertex, start_object);
}

void dx12command::DispatchRays(D3D12_DISPATCH_RAYS_DESC* description)
{
	m_command_list->DispatchRays(description);
}

void dx12command::BuildRaytracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* desc)
{
	m_command_list->BuildRaytracingAccelerationStructure(desc, 0, nullptr);
}
