#include "dx12texturemanager.h"
#include "dx12core.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "dx12core.h"

#include <iostream>

using Microsoft::WRL::ComPtr;

TextureInfo dx12texturemanager::LoadTextureFromFile(const std::string& texture_file_name, UINT channels) const
{
	int width, height, comp;
	unsigned char* imageData = stbi_load(texture_file_name.c_str(),
		&width, &height, &comp, channels);
	TextureInfo texture = {imageData, width, height, comp, channels};
	return texture;
}

void dx12texturemanager::UploadTextureData(ID3D12Resource* target_resource, unsigned char* data, UINT alignment)
{
	int subresource = 0;

	D3D12_RANGE nothing = { 0, 0 };
	unsigned char* mapped_ptr = nullptr;
	HRESULT hr = m_upload_buffer->Map(0, &nothing, reinterpret_cast<void**>(&mapped_ptr));
	assert(SUCCEEDED(hr));

	D3D12_RESOURCE_DESC resource_desc = target_resource->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT nr_of_rows = 0;
	UINT64 row_size_in_bytes = 0;
	UINT64 total_bytes = 0;
	dx12core::GetDx12Core().GetDevice()->GetCopyableFootprints(&resource_desc, subresource, 1, 0, &footprint, &nr_of_rows,
		&row_size_in_bytes, &total_bytes);

	unsigned int source_offset = 0;
	size_t destination_offset = ((m_upload_current_offset + (alignment - 1)) & ~(alignment - 1));//AlignTextureAdress(m_current_upload_offset, alignment);

	for (UINT row = 0; row < nr_of_rows; ++row)
	{
		std::memcpy(mapped_ptr + destination_offset, data + source_offset, row_size_in_bytes);
		source_offset += row_size_in_bytes;
		destination_offset += footprint.Footprint.RowPitch;
	}

	D3D12_TEXTURE_COPY_LOCATION destination;
	destination.pResource = target_resource;
	destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destination.SubresourceIndex = subresource;
	D3D12_TEXTURE_COPY_LOCATION source;
	source.pResource = m_upload_buffer.Get();
	source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	source.PlacedFootprint.Offset = ((m_upload_current_offset + (alignment - 1)) & ~(alignment - 1));//AlignTextureAdress(m_current_upload_offset, alignment);
	source.PlacedFootprint.Footprint.Width = footprint.Footprint.Width;
	source.PlacedFootprint.Footprint.Height = footprint.Footprint.Height;
	source.PlacedFootprint.Footprint.Depth = footprint.Footprint.Depth;
	source.PlacedFootprint.Footprint.RowPitch = footprint.Footprint.RowPitch;
	source.PlacedFootprint.Footprint.Format = resource_desc.Format;

	dx12core::GetDx12Core().GetDirectCommand()->CopyTextureRegion(&destination, &source);
	m_upload_buffer->Unmap(0, nullptr);

	m_upload_current_offset = destination_offset;
}

Microsoft::WRL::ComPtr<ID3D12Resource> dx12texturemanager::CreateCommitedTexture(const TextureType& texture_type, const TextureInfo& texture_info, D3D12_CLEAR_VALUE* clear_value)
{
	D3D12_HEAP_PROPERTIES heapProperties;
	ZeroMemory(&heapProperties, sizeof(heapProperties));
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = texture_info.width;
	desc.Height = texture_info.height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = static_cast<UINT16>(1);//textureInfo.mipLevels);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE; // D3D12_RESOURCE_FLAG_
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//desc.Flags = (D3D12_RESOURCE_FLAGS)TranslateBindFlags(textureInfo.bindingFlags);

	//Set flags
	if (texture_type & TextureType::TEXTURE_UAV)
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	if (texture_type & TextureType::TEXTURE_RTV)
		desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	ComPtr<ID3D12Resource> texture;
	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON,
		clear_value, IID_PPV_ARGS(texture.GetAddressOf()));

	assert(SUCCEEDED(hr));

	return texture;
}

TextureResource dx12texturemanager::CreateTextureViews(ComPtr<ID3D12Resource> texture, const TextureType& texture_type)
{
	TextureResource new_texture_resource;
	if (TextureType::TEXTURE_SRV & texture_type)
	{
		if (m_dh_shader_bindable_current_offset >= m_max_dh_shader_bindable_offset)
			assert(false);

		UINT descriptor_size_shader_bindable = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_CPU_DESCRIPTOR_HANDLE heap_handle = m_dh_shader_bindable->GetCPUDescriptorHandleForHeapStart();
		heap_handle.ptr += descriptor_size_shader_bindable * m_dh_shader_bindable_current_offset;

		dx12core::GetDx12Core().GetDevice()->CreateShaderResourceView(texture.Get(), nullptr, heap_handle);

		++m_dh_shader_bindable_current_offset;

		dx12texture texture_data = {};
		texture_data.descriptor_heap_offset = m_dh_shader_bindable_current_offset - 1;
		texture_data.resource_index = m_view_resources.size();
		texture_data.texture_type = TextureType::TEXTURE_SRV;

		new_texture_resource.shader_resource_view = texture_data;
	}
	if (TextureType::TEXTURE_UAV & texture_type)
	{
		if (m_dh_shader_bindable_current_offset >= m_max_dh_shader_bindable_offset)
			assert(false);

		UINT descriptor_size_shader_bindable = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_CPU_DESCRIPTOR_HANDLE heap_handle = m_dh_shader_bindable->GetCPUDescriptorHandleForHeapStart();
		heap_handle.ptr += descriptor_size_shader_bindable * m_dh_shader_bindable_current_offset;

		dx12core::GetDx12Core().GetDevice()->CreateUnorderedAccessView(texture.Get(), nullptr, nullptr, heap_handle);

		++m_dh_shader_bindable_current_offset;

		dx12texture texture_data = {};
		texture_data.descriptor_heap_offset = m_dh_shader_bindable_current_offset - 1;
		texture_data.resource_index = m_view_resources.size();
		texture_data.texture_type = TextureType::TEXTURE_UAV;

		new_texture_resource.unordered_access_view = texture_data;
	}
	if (TextureType::TEXTURE_RTV & texture_type)
	{
		if (m_dh_RTV_current_offset >= m_max_dh_RTV_offset)
			assert(false);

		UINT descriptor_size_render_target_view = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_CPU_DESCRIPTOR_HANDLE heap_handle = m_dh_RTV->GetCPUDescriptorHandleForHeapStart();
		heap_handle.ptr += descriptor_size_render_target_view * m_dh_RTV_current_offset;

		dx12core::GetDx12Core().GetDevice()->CreateRenderTargetView(texture.Get(), nullptr, heap_handle);

		++m_dh_RTV_current_offset;

		dx12texture texture_data = {};
		texture_data.descriptor_heap_offset = m_dh_RTV_current_offset - 1;
		texture_data.resource_index = m_view_resources.size();
		texture_data.texture_type = TextureType::TEXTURE_RTV;

		new_texture_resource.render_target_view = texture_data;
	}
	return new_texture_resource;
}

dx12texturemanager::dx12texturemanager(UINT max_render_target_views, UINT max_depth_stencil_views, UINT max_shader_bindables)
{
	const UINT upload_buffer_size = 100000000;

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


	//Create upload heap and buffer
	D3D12_HEAP_PROPERTIES properties;
	properties.Type = D3D12_HEAP_TYPE_UPLOAD;
	properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CreationNodeMask = 0;
	properties.VisibleNodeMask = 0;

	D3D12_HEAP_DESC heap_desc;
	heap_desc.SizeInBytes = upload_buffer_size;
	heap_desc.Properties = properties;
	heap_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	heap_desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

	hr = dx12core::GetDx12Core().GetDevice()->CreateHeap(&heap_desc, IID_PPV_ARGS(m_upload_heap.GetAddressOf()));
	assert(SUCCEEDED(hr));

	D3D12_RESOURCE_DESC buffer_desc;
	buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	buffer_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	buffer_desc.Width = upload_buffer_size;
	buffer_desc.Height = 1;
	buffer_desc.DepthOrArraySize = 1;
	buffer_desc.MipLevels = 1;
	buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
	buffer_desc.SampleDesc.Count = 1;
	buffer_desc.SampleDesc.Quality = 0;
	buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = dx12core::GetDx12Core().GetDevice()->CreatePlacedResource(m_upload_heap.Get(), 0, &buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(m_upload_buffer.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

dx12texturemanager::~dx12texturemanager()
{
}

TextureResource dx12texturemanager::CreateRenderTargetView(UINT back_buffer_index)
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

	m_view_resources.push_back(back_buffer);

	dx12texture render_target = {};
	render_target.descriptor_heap_offset = m_dh_RTV_current_offset - 1;
	render_target.resource_index = m_view_resources.size() - 1;
	render_target.texture_type = TextureType::TEXTURE_RTV;

	TextureResource new_texture_resource;
	new_texture_resource.render_target_view = render_target;

	return new_texture_resource;
}

TextureResource dx12texturemanager::CreateDepthStencilView(UINT texture_width, UINT texture_height, D3D12_RESOURCE_FLAGS flags)
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

	m_view_resources.push_back(depth_stencil);

	dx12texture depth_stencil_data = {};
	depth_stencil_data.descriptor_heap_offset = m_dh_DSV_current_offset - 1;
	depth_stencil_data.resource_index = m_view_resources.size() - 1;
	depth_stencil_data.texture_type = TextureType::TEXTURE_DSV;

	TextureResource new_texture_resource;
	new_texture_resource.depth_stencil_view = depth_stencil_data;

	return new_texture_resource;
}

TextureResource dx12texturemanager::CreateTexture2D(const std::string& texture_file_name, const TextureType& texture_type)
{
	TextureInfo texture_info = LoadTextureFromFile(texture_file_name);

	ComPtr<ID3D12Resource> texture = CreateCommitedTexture(texture_type, texture_info);

	//Upload texture data
	UploadTextureData(texture.Get(), texture_info.texture_data, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

	TextureResource new_texture_resource;

	//Create specific views
	new_texture_resource = CreateTextureViews(texture, texture_type);

	m_view_resources.push_back(texture);

	//Texture needs transition. Promoted common -> copy_dest but does not decay back
	//Fix later that we can only upload textures to the pixel shader, maybe??
	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
	return new_texture_resource;
}

TextureResource dx12texturemanager::CreateTexture2D(UINT texture_width, UINT texture_height, const TextureType& texture_type, D3D12_CLEAR_VALUE* clear_value)
{
	TextureInfo texture_info;
	texture_info.width = texture_width;
	texture_info.height = texture_height;

	ComPtr<ID3D12Resource> texture = CreateCommitedTexture(texture_type, texture_info, clear_value);

	TextureResource new_texture_resource;

	//Create specific views
	new_texture_resource = CreateTextureViews(texture, texture_type);

	m_view_resources.push_back(texture);

	return new_texture_resource;
}

dx12texture dx12texturemanager::CreateStructuredBuffer(ID3D12Resource* resource, UINT element_size, UINT nr_of_elements, TextureType texture_type)
{
	UINT descriptor_size_shader_bindable = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	dx12texture structured_buffer = {};

	if (texture_type & TextureType::TEXTURE_SRV)
	{
		//Create shader resource view
		D3D12_CPU_DESCRIPTOR_HANDLE heap_handle = m_dh_shader_bindable->GetCPUDescriptorHandleForHeapStart();
		heap_handle.ptr += descriptor_size_shader_bindable * m_dh_shader_bindable_current_offset;

		D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
		shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
		shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shader_resource_view_desc.Buffer.NumElements = nr_of_elements;
		shader_resource_view_desc.Buffer.FirstElement = 0;
		shader_resource_view_desc.Buffer.StructureByteStride = element_size;
		shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		dx12core::GetDx12Core().GetDevice()->CreateShaderResourceView(resource, &shader_resource_view_desc, heap_handle);

		structured_buffer.descriptor_heap_offset = m_dh_shader_bindable_current_offset;
		structured_buffer.resource_index = -1;
		structured_buffer.texture_type = TextureType::TEXTURE_CBV;

		++m_dh_shader_bindable_current_offset;
	}
	else if (texture_type & TextureType::TEXTURE_UAV)
	{
		//Create unorded access view
		D3D12_CPU_DESCRIPTOR_HANDLE heap_handle = m_dh_shader_bindable->GetCPUDescriptorHandleForHeapStart();
		heap_handle.ptr += descriptor_size_shader_bindable * (m_max_dh_shader_bindable_offset - 1);
		heap_handle.ptr -= descriptor_size_shader_bindable * m_acceleration_counter;

		D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
		shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		shader_resource_view_desc.RaytracingAccelerationStructure.Location = resource->GetGPUVirtualAddress();
		shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
		//shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
		//shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		//shader_resource_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		//shader_resource_view_desc.Buffer.NumElements = nr_of_elements;
		//shader_resource_view_desc.Buffer.FirstElement = 0;
		//shader_resource_view_desc.Buffer.StructureByteStride = element_size;
		//shader_resource_view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		dx12core::GetDx12Core().GetDevice()->CreateShaderResourceView(nullptr, &shader_resource_view_desc, heap_handle);

		//D3D12_UNORDERED_ACCESS_VIEW_DESC unorded_access_view_desc = {};
		//unorded_access_view_desc.Format = DXGI_FORMAT_UNKNOWN;
		//unorded_access_view_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER; 
		//unorded_access_view_desc.Buffer.NumElements = nr_of_elements;
		//unorded_access_view_desc.Buffer.FirstElement = 0;
		//unorded_access_view_desc.Buffer.StructureByteStride = element_size;
		//unorded_access_view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		//dx12core::GetDx12Core().GetDevice()->CreateUnorderedAccessView(resource, nullptr, &unorded_access_view_desc, heap_handle);

		structured_buffer.descriptor_heap_offset = m_max_dh_shader_bindable_offset - m_acceleration_counter - 1;
		structured_buffer.resource_index = -1;
		structured_buffer.texture_type = TextureType::TEXTURE_CBV;

		++m_acceleration_counter;
	}

	return structured_buffer;
}

ID3D12Resource* dx12texturemanager::GetTextureResource(UINT resource_index)
{
	return m_view_resources[resource_index].Get();
}

ID3D12DescriptorHeap* dx12texturemanager::GetShaderBindableDescriptorHeap()
{
	return m_dh_shader_bindable.Get();
}

ID3D12DescriptorHeap* dx12texturemanager::GetRenderTargetViewDescriptorHeap()
{
	return m_dh_RTV.Get();
}

ID3D12DescriptorHeap* dx12texturemanager::GetDepthStencilViewDescriptorHeap()
{
	return m_dh_DSV.Get();
}

//Imorgon
//Add upload buffer for data and make it so we do not use 1.4 gb just for some textures
//Root parameters
//Draw a triangle 

TextureType operator|(TextureType lhs, TextureType rhs)
{
	return TextureType((UINT16)lhs | (UINT16)rhs);
}

UINT16 operator&(const TextureType& lhs, const TextureType& rhs)
{
	return (UINT16)lhs & (UINT16)rhs;
}

TextureResource::TextureResource()
{
	shader_resource_view.descriptor_heap_offset = -1;
	shader_resource_view.resource_index = -1;

	render_target_view.descriptor_heap_offset = -1;
	render_target_view.resource_index = -1;

	depth_stencil_view.descriptor_heap_offset = -1;
	depth_stencil_view.resource_index = -1;

	unordered_access_view.descriptor_heap_offset = -1;
	unordered_access_view.resource_index = -1;
}
