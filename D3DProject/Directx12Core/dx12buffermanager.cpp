#include "dx12buffermanager.h"
#include "dx12core.h"

void dx12buffermanager::UploadBufferData(ID3D12Resource* upload_resource, unsigned char* data, UINT alignment)
{
	D3D12_RANGE nothing = { 0, 0 };
	unsigned char* mapped_ptr = nullptr;
	HRESULT hr = m_upload_buffer->Map(0, &nothing, reinterpret_cast<void**>(&mapped_ptr));
	assert(SUCCEEDED(hr));

	D3D12_RESOURCE_DESC resourceDesc = upload_resource->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT nr_of_rows = 0;
	UINT64 row_size_in_bytes = 0;
	UINT64 total_bytes = 0;
	dx12core::GetDx12Core().GetDevice()->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &footprint, &nr_of_rows,
		&row_size_in_bytes, &total_bytes);

	unsigned int source_offset = 0;

	//UINT buffer_upload_offset = update_upload_offset;
	//If upload_offset is -1 then we are creating the buffer for the first time
	//if (buffer_upload_offset == -1)
		//buffer_upload_offset = m_current_upload_offset;

	size_t destination_offset = ((m_upload_current_offset + (alignment - 1)) & ~(alignment - 1));//AlignAdress(buffer_upload_offset, alignment);

	for (UINT row = 0; row < nr_of_rows; ++row)
	{
		std::memcpy(mapped_ptr + destination_offset, data + source_offset, row_size_in_bytes);
		source_offset += row_size_in_bytes;
		destination_offset += footprint.Footprint.RowPitch;
	}

	dx12core::GetDx12Core().GetDirectCommand()->CopyBufferRegion(upload_resource, m_upload_buffer.Get(), 0, ((m_upload_current_offset + (alignment - 1)) & ~(alignment - 1)), resourceDesc.Width);

	//dx12core::GetDx12Core().GetDirectCommand()->CopyBufferRegion(upload_resource, 0, m_upload_buffer,
	//	((m_upload_current_offset + (alignment - 1)) & ~(alignment - 1)), resourceDesc.Width);

	m_upload_buffer->Unmap(0, nullptr);

	//Transition the buffer to common state
	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(upload_resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	//m_renderer->AddTransitionResource(upload_resource, m_renderer->GetCurrentDirectCommand()->cmd_list, D3D12_RESOURCE_STATE_COPY_DEST,
		//D3D12_RESOURCE_STATE_COMMON);

	//If upload_offset is not -1 then we are updating the buffer
	//And do not need to update the upload offset
	m_upload_current_offset = destination_offset;
}

dx12buffermanager::dx12buffermanager(dx12texturemanager* texture_manager)
{
	m_texture_manager = texture_manager;

	//Create upload heap and buffer
	D3D12_HEAP_PROPERTIES properties;
	properties.Type = D3D12_HEAP_TYPE_UPLOAD;
	properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CreationNodeMask = 0;
	properties.VisibleNodeMask = 0;

	D3D12_HEAP_DESC heap_desc;
	heap_desc.SizeInBytes = 1000000;
	heap_desc.Properties = properties;
	heap_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	//Only allow buffers for this heap
	heap_desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateHeap(&heap_desc, IID_PPV_ARGS(m_upload_heap.GetAddressOf()));
	assert(SUCCEEDED(hr));

	D3D12_RESOURCE_DESC buffer_desc;
	buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	buffer_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	buffer_desc.Width = 1000000;
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

BufferResource dx12buffermanager::CreateBuffer(void* data, unsigned int elementSize, unsigned int nrOfElements)
{
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = elementSize * nrOfElements;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heap_properties;
	ZeroMemory(&heap_properties, sizeof(heap_properties));
	heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

	Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateCommittedResource(&heap_properties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(buffer.GetAddressOf()));
	assert(SUCCEEDED(hr));

	UploadBufferData(buffer.Get(), (unsigned char*)data, elementSize);

	BufferResource buffer_resource;
	buffer_resource.buffer = buffer;
	return buffer_resource;
}

BufferResource dx12buffermanager::CreateStructuredBuffer(void* data, unsigned int elementSize, unsigned int nrOfElements)
{
	BufferResource buffer_resource = CreateBuffer(data, elementSize, nrOfElements);
	buffer_resource.structured_buffer = m_texture_manager->CreateStructuredBuffer(buffer_resource.buffer.Get(), elementSize, nrOfElements);
	return BufferResource();
}

BufferResource::BufferResource()
{
	structured_buffer.resource_index = -1;
	structured_buffer.resource_index = -1;
}
