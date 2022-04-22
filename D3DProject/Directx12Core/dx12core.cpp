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

	ID3D12Device* temp_device = nullptr;
	hr = D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&temp_device));
	assert(SUCCEEDED(hr));
	hr = temp_device->QueryInterface(__uuidof(ID3D12Device5),
		reinterpret_cast<void**>(m_device.GetAddressOf()));
	temp_device->Release();
	assert(SUCCEEDED(hr));
}

void dx12core::CreateSwapchain(HWND window_handle)
{
	DXGI_SWAP_CHAIN_DESC1 desc;
	desc.Width = 0;
	desc.Height = 0;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = false;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = m_backbuffer_count;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = 0;

	IDXGISwapChain1* swap_chain;
	HRESULT hr = m_factory->CreateSwapChainForHwnd(m_direct_command_queue.Get(), window_handle, &desc, nullptr, nullptr, &swap_chain);
	assert(SUCCEEDED(hr));

	//Change the Swapchain from IDXGISwapChain1 to IDXGISwapChain3
	hr = swap_chain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)m_swapchain.GetAddressOf());
	assert(SUCCEEDED(hr));
	swap_chain->Release();

	m_swapchain->GetDesc1(&desc);

	//Getting the size of the window
	m_backbuffer_width = desc.Width;
	m_backbuffer_height = desc.Height;
}

void dx12core::CreateCommandQueue(ID3D12CommandQueue** command_queue, const D3D12_COMMAND_LIST_TYPE& command_type)
{
	D3D12_COMMAND_QUEUE_DESC desc;
	desc.Type = command_type;
	desc.Priority = 0;
	desc.NodeMask = 0;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	HRESULT hr = m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(command_queue));
	assert(SUCCEEDED(hr));
}

dx12core::dx12core()
{

}

void dx12core::PreDraw()
{
	m_direct_command->Reset();

	UINT current_backbuffer_index = m_swapchain->GetCurrentBackBufferIndex();

	// Set descriptor heaps before setting root signature if direct heap access is needed
	m_direct_command->SetDescriptorHeap(m_texture_manager->GetShaderBindableDescriptorHeap());
	m_direct_command->SetRootSignature(m_render_pipeline->GetRootSignature());
	m_direct_command->SetPipelineState(m_render_pipeline->GetPipelineState());

	ID3D12Resource* back_buffer = m_texture_manager->GetTextureResource(m_backbuffers[current_backbuffer_index].render_target_view.resource_index);
	m_direct_command->TransistionBuffer(back_buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_direct_command->ClearRenderTargetView(m_texture_manager->GetRenderTargetViewDescriptorHeap(), current_backbuffer_index);
	m_direct_command->ClearDepthStencilView(m_texture_manager->GetDepthStencilViewDescriptorHeap(), 0);

	m_direct_command->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_direct_command->SetOMRenderTargets(m_texture_manager->GetRenderTargetViewDescriptorHeap(), current_backbuffer_index, m_texture_manager->GetDepthStencilViewDescriptorHeap(), 0);
	m_direct_command->SetViewport(m_backbuffer_width, m_backbuffer_height);
	m_direct_command->SetScissorRect(m_backbuffer_width, m_backbuffer_height);
}

void dx12core::Draw()
{
	auto object_binds = m_render_pipeline->GetObjects();
	for (auto& object : object_binds)
	{
		auto object_buffers = object->GetBuffers();
		for (auto& buffer : object_buffers)
		{
			switch (buffer.second->binding_type)
			{
				case BindingType::STRUCTURED_BUFFER:
					m_direct_command->SetDescriptorTable(buffer.second, m_texture_manager->GetShaderBindableDescriptorHeap(), buffer.first.structured_buffer);
					break;
			}
		}

		auto object_textures = object->GetTextures();
		for (auto& texture : object_textures)
		{
			switch (texture.second->binding_type)
			{
			case BindingType::SHADER_RESOURCE:
				m_direct_command->SetDescriptorTable(texture.second, m_texture_manager->GetShaderBindableDescriptorHeap(), texture.first.shader_resource_view);
				break;
			}
		}

		m_direct_command->Draw(3, 1, 0, 0);
	}
	//m_direct_command->SetDescriptorTable(m_render_pipeline->GetObjectRootBinds()[0], m_texture_manager->GetShaderBindableDescriptorHeap(), vertex.structured_buffer);
	//m_direct_command->SetDescriptorTable(m_render_pipeline->GetObjectRootBinds()[1], m_texture_manager->GetShaderBindableDescriptorHeap(), texture.shader_resource_view);
	//commandList->SetGraphicsRootShaderResourceView(0, vertexBuffer->GetGPUVirtualAddress());
	//commandList->SetGraphicsRootDescriptorTable(1, shaderBindableHeap->GetGPUDescriptorHandleForHeapStart());
	//commandList->DrawInstanced(3, 1, 0, 0);
}

void dx12core::FinishDraw()
{
	UINT current_backbuffer_index = m_swapchain->GetCurrentBackBufferIndex();
	ID3D12Resource* back_buffer = m_texture_manager->GetTextureResource(m_backbuffers[current_backbuffer_index].render_target_view.resource_index);
	m_direct_command->TransistionBuffer(back_buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	m_direct_command->Execute();
	m_swapchain->Present(0,0);
	m_direct_command->SignalAndWait();
}

dx12core::~dx12core()
{
	//m_adapter->Release();
	//m_device->Release();
	//m_factory->Release();

	delete m_direct_command;
	delete m_texture_manager;
}

dx12core& dx12core::GetDx12Core()
{
	return s_instance;
}

void dx12core::Init(HWND hwnd, UINT backbuffer_count)
{
	m_backbuffer_count = backbuffer_count;

	if (m_backbuffer_count < 2)
		assert(false);

	EnableDebugLayer();
	EnableGPUBasedValidation();

	CreateDevice();
	CreateCommandQueue(m_direct_command_queue.GetAddressOf(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	CreateSwapchain(hwnd);

	m_direct_command = new dx12command(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_texture_manager = new dx12texturemanager(2, 1, 50);
	m_buffer_manager = new dx12buffermanager(m_texture_manager);

	for (int i = 0; i < m_backbuffer_count; ++i)
		m_backbuffers.push_back(m_texture_manager->CreateRenderTargetView(i));

	m_depth_stencil = m_texture_manager->CreateDepthStencilView(m_backbuffer_width, m_backbuffer_height, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

	//m_texture_manager->CreateTexture2D("Textures/image.png", TextureType::TEXTURE_SRV);


}

ID3D12Device5* dx12core::GetDevice()
{
	return m_device.Get();
}

IDXGISwapChain3* dx12core::GetSwapChain()
{
	return m_swapchain.Get();
}

dx12command* dx12core::GetDirectCommand()
{
	return m_direct_command;
}

dx12texturemanager* dx12core::GetTextureManager()
{
	return m_texture_manager;
}

dx12buffermanager* dx12core::GetBufferManager()
{
	return m_buffer_manager;
}

ID3D12CommandQueue* dx12core::GetCommandQueue()
{
	return m_direct_command_queue.Get();
}

void dx12core::SetRenderPipeline(dx12renderpipeline* render_pipeline)
{
	m_render_pipeline = render_pipeline;
}

void dx12core::Show()
{
	//PreDraw();
	//Draw();
	//FinishDraw();
}
