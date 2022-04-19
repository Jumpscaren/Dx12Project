#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>

class dx12core
{
private:
	Microsoft::WRL::ComPtr<IDXGIFactory5> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dh_RTV;

	static dx12core s_instance;
private:
	void EnableDebugLayer();
	void EnableGPUBasedValidation();
	bool CheckDXRSupport(ID3D12Device* device);
	void CreateDevice();
	void CreateRenderTargetViews();
	dx12core();
public:
	~dx12core();
	dx12core(const dx12core& other) = delete;
	dx12core& operator=(const dx12core& other) = delete;
	static dx12core& GetDx12Core();
};
