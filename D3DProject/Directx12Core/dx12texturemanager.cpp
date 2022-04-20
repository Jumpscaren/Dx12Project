#include "dx12texturemanager.h"
#include "dx12core.h"

using Microsoft::WRL::ComPtr;

dx12texturemanager::dx12texturemanager(UINT max_render_target_views, UINT max_depth_stencil_views, UINT max_shader_bindables)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.NumDescriptors = max_render_target_views;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;

	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_dh_RTV.GetAddressOf()));
	assert(SUCCEEDED(hr));

	m_dh_RTV_current_offset = 0;
	m_max_dh_RTV_offset = max_render_target_views;

	desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	desc.NumDescriptors = max_depth_stencil_views;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;

	hr = dx12core::GetDx12Core().GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_dh_DSV.GetAddressOf()));
	assert(SUCCEEDED(hr));

	m_dh_DSV_current_offset = 0;
	m_max_dh_DSV_offset = max_depth_stencil_views;

	desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = max_shader_bindables;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;

	hr = dx12core::GetDx12Core().GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_dh_shader_bindable.GetAddressOf()));
	assert(SUCCEEDED(hr));

	m_dh_shader_bindable_current_offset = 0;
	m_max_dh_shader_bindable_offset = max_shader_bindables;
}

dx12texturemanager::~dx12texturemanager()
{
}

dx12texture dx12texturemanager::CreateRenderTargetView(UINT back_buffer_index)
{
	assert(m_dh_RTV_current_offset < m_max_dh_RTV_offset);

	UINT descriptor_size_RTV = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE heap_handle =
		m_dh_RTV->GetCPUDescriptorHandleForHeapStart();
	heap_handle.ptr += descriptor_size_RTV * m_dh_RTV_current_offset;

	ComPtr<ID3D12Resource> back_buffer = nullptr;
	HRESULT hr = dx12core::GetDx12Core().GetSwapChain()->GetBuffer(back_buffer_index, IID_PPV_ARGS(back_buffer.GetAddressOf()));
	assert(SUCCEEDED(hr));

	dx12core::GetDx12Core().GetDevice()->CreateRenderTargetView(back_buffer.Get(), nullptr, heap_handle);

	++m_dh_RTV_current_offset;

	m_render_target_view_resources.push_back(back_buffer);

	dx12texture render_target = {};
	render_target.descriptor_heap_offset = m_dh_RTV_current_offset - 1;
	render_target.resource_index = m_render_target_view_resources.size() - 1;
	render_target.texture_type = TextureType::TEXTURE_RTV;

	return render_target;
}

dx12texture dx12texturemanager::CreateDepthStencilView(UINT texture_width, UINT texture_height, D3D12_RESOURCE_FLAGS flags)
{
	assert(m_dh_DSV_current_offset < m_max_dh_DSV_offset);

	D3D12_CLEAR_VALUE depth_clear_value;
	depth_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
	depth_clear_value.DepthStencil.Depth = 1.0f;

	D3D12_HEAP_PROPERTIES heap_properties;
	ZeroMemory(&heap_properties, sizeof(heap_properties));
	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = texture_width;
	desc.Height = texture_height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = static_cast<UINT16>(1);
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | flags;//D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

	ComPtr<ID3D12Resource> depth_stencil = nullptr;
	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON,
		&depth_clear_value, IID_PPV_ARGS(depth_stencil.GetAddressOf()));
	assert(SUCCEEDED(hr));

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(depth_stencil.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	//AddTransitionResource(m_depth_stencil, m_direct_commands[0].cmd_list, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	UINT descriptor_size_DSV = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_CPU_DESCRIPTOR_HANDLE heap_handle = m_dh_DSV->GetCPUDescriptorHandleForHeapStart();
	heap_handle.ptr += descriptor_size_DSV * m_dh_DSV_current_offset;
	dx12core::GetDx12Core().GetDevice()->CreateDepthStencilView(depth_stencil.Get(), nullptr, heap_handle);

	++m_dh_DSV_current_offset;

	m_depth_stencil_view_resources.push_back(depth_stencil);

	dx12texture depth_stencil_data = {};
	depth_stencil_data.descriptor_heap_offset = m_dh_DSV_current_offset - 1;
	depth_stencil_data.resource_index = m_depth_stencil_view_resources.size() - 1;
	depth_stencil_data.texture_type = TextureType::TEXTURE_DSV;

	return depth_stencil_data;
}

//Imorgon
//Add upload buffer for data and make it so we do not use 1.4 gb just for some textures
//Root parameters
//Draw a triangle 