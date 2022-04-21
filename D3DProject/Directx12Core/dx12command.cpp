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

void dx12command::CopyTextureRegion(D3D12_TEXTURE_COPY_LOCATION* destination, D3D12_TEXTURE_COPY_LOCATION* source)
{
	m_command_list->CopyTextureRegion(destination, 0, 0, 0, source, nullptr);
}

void dx12command::CopyBufferRegion(ID3D12Resource* destination_buffer, ID3D12Resource* source_buffer, UINT destination_offset, UINT source_offset, UINT size)
{
	m_command_list->CopyBufferRegion(destination_buffer, destination_offset, source_buffer, source_offset, size);
}
