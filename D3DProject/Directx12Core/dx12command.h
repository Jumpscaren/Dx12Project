#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>

class dx12command
{
private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_allocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fence_value;

	const UINT64 c_initial_value = 0;
public:
	dx12command(const D3D12_COMMAND_LIST_TYPE& command_type);
	~dx12command();
	void TransistionBuffer(ID3D12Resource* resource, D3D12_RESOURCE_STATES before_state, D3D12_RESOURCE_STATES after_state);
};

