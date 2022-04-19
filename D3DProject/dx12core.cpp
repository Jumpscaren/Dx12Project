#include "dx12core.h"

using Microsoft::WRL::ComPtr;

dx12core dx12core::s_instance = dx12core();

void dx12core::EnableDebugLayer()
{
	ComPtr<ID3D12Debug> debugController;
	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()));
	assert(SUCCEEDED(hr));
	debugController->EnableDebugLayer();
	debugController.Get()->Release();
}

void dx12core::EnableGPUBasedValidation()
{
	ComPtr<ID3D12Debug1> debugController;
	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()));
	assert(SUCCEEDED(hr));
	debugController->SetEnableGPUBasedValidation(true);
	debugController.Get()->Release();
}

bool dx12core::CheckDXRSupport(ID3D12Device* device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureData = {};
	HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureData,
		sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

	if (FAILED(hr))
		return false;

	return featureData.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
}

void dx12core::CreateDevice()
{
	//Create factory
	UINT factory_flags = 0;
	ComPtr<IDXGIFactory5> temp_factory;
	HRESULT hr = CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(temp_factory.GetAddressOf()));
	assert(SUCCEEDED(hr));
	hr = temp_factory->QueryInterface(__uuidof(IDXGIFactory5), (void**)(m_factory.GetAddressOf()));
	assert(SUCCEEDED(hr));

	m_adapter = nullptr;
	IDXGIAdapter1* temp_adapter = nullptr;
	UINT adapter_index = 0;

	while (hr != DXGI_ERROR_NOT_FOUND)
	{
		hr = m_factory->EnumAdapters1(adapter_index, &temp_adapter);
		//Wait until we can not find a adapter
		if (FAILED(hr))
			continue;
		ComPtr<ID3D12Device> temp_device;
		hr = D3D12CreateDevice(temp_adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)temp_device.GetAddressOf());

		if (FAILED(hr) || CheckDXRSupport(temp_device.Get()))
		{
			temp_adapter->Release();
			temp_adapter = nullptr;
		}
		else
		{
			if (!m_adapter)
			{
				m_adapter = temp_adapter;
			}
			//Compare which of the adapters has the most vram and take the one with the most
			else
			{
				DXGI_ADAPTER_DESC1 temp_adapter_desc;
				DXGI_ADAPTER_DESC1 adapter_desc;
				m_adapter->GetDesc1(&adapter_desc);
				temp_adapter->GetDesc1(&temp_adapter_desc);

				if (adapter_desc.DedicatedVideoMemory < temp_adapter_desc.DedicatedVideoMemory)
				{
					m_adapter.Get()->Release();
					m_adapter = temp_adapter;
				}
				else
				{
					temp_adapter->Release();
					temp_adapter = nullptr;
				}
			}
		}
		++adapter_index;
	}

	hr = D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_device.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

//For next time
//Create swapchain and render target views

void dx12core::CreateRenderTargetViews()
{
	//UINT descriptor_size_RTV = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//D3D12_DESCRIPTOR_HEAP_DESC desc;
	//desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	//desc.NumDescriptors = m_number_of_back_buffers;
	//desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	//desc.NodeMask = 0;

	//HRESULT hr = m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_dh_RTV.GetAddressOf()));
	//assert(SUCCEEDED(hr));

	//UINT descriptor_start_offset = 0;

	//D3D12_CPU_DESCRIPTOR_HANDLE heap_handle =
	//	m_dh_RTV->GetCPUDescriptorHandleForHeapStart();
	//heap_handle.ptr += descriptor_size_RTV * descriptor_start_offset;

	//for (int i = 0; i < m_number_of_back_buffers; ++i)
	//{
	//	ID3D12Resource* back_buffer = nullptr;
	//	HRESULT hr = m_swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));
	//	assert(SUCCEEDED(hr));
	//	m_device->CreateRenderTargetView(back_buffer, nullptr, heap_handle);
	//	heap_handle.ptr += descriptor_size_RTV;
	//	m_back_buffers.push_back(back_buffer);
	//}
}

dx12core::dx12core()
{
	EnableDebugLayer();
	EnableGPUBasedValidation();

	CreateDevice();
}

dx12core::~dx12core()
{
	//m_adapter->Release();
	//m_device->Release();
	//m_factory->Release();
}

dx12core& dx12core::GetDx12Core()
{
	return s_instance;
}
