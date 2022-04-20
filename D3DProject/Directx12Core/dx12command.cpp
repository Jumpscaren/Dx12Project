#include "dx12command.h"
#include "dx12core.h"

dx12command::dx12command(const D3D12_COMMAND_LIST_TYPE& command_type)
{
	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateCommandAllocator(command_type, IID_PPV_ARGS(m_command_allocator.GetAddressOf()));
	assert(SUCCEEDED(hr));

	hr = dx12core::GetDx12Core().GetDevice()->CreateCommandList(0, command_type, m_command_allocator.Get(),
		nullptr, IID_PPV_ARGS(m_command_list.GetAddressOf()));
	assert(SUCCEEDED(hr));

	hr = dx12core::GetDx12Core().GetDevice()->CreateFence(c_initial_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));
	assert(SUCCEEDED(hr));

	m_fence_value = c_initial_value;
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
