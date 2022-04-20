#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>
#include <vector>
#include "dx12command.h"
#include "dx12texturemanager.h"

class dx12core
{
private:
	Microsoft::WRL::ComPtr<IDXGIFactory5> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dh_RTV;
	UINT m_dh_RTV_current_offset;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dh_DSV;
	UINT m_dh_DSV_current_offset;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_direct_command_queue;
	dx12command* m_direct_command;
	
	UINT m_backbuffer_count;
	UINT m_backbuffer_width, m_backbuffer_height;
	std::vector<dx12texture> m_backbuffers;

	dx12texturemanager* m_texture_manager;

	dx12texture m_depth_stencil;

private:
	static dx12core s_instance;
private:
	void EnableDebugLayer();
	void EnableGPUBasedValidation();
	bool CheckDXRSupport(ID3D12Device* device);
	void CreateDevice();
	void CreateSwapchain(HWND window_handle);
	void CreateCommandQueue(ID3D12CommandQueue** command_queue, const D3D12_COMMAND_LIST_TYPE& command_type);
	dx12core();
public:
	~dx12core();
	dx12core(const dx12core& other) = delete;
	dx12core& operator=(const dx12core& other) = delete;
	static dx12core& GetDx12Core();
	void Init(HWND hwnd, UINT backbuffer_count = 2);
	ID3D12Device* GetDevice();
	IDXGISwapChain3* GetSwapChain();
	dx12command* GetDirectCommand();
};
