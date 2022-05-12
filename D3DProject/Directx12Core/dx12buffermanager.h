#pragma once
#include "dx12texturemanager.h"

struct BufferResource
{
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
	dx12texture structured_buffer;

	BufferResource();

	UINT element_size;
	UINT nr_of_elements;
};

class dx12buffermanager
{
private:
	dx12texturemanager* m_texture_manager;

	Microsoft::WRL::ComPtr<ID3D12Heap> m_upload_heap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_buffer;
	UINT64 m_upload_current_offset;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_temp;

	void UploadBufferData(ID3D12Resource* upload_resource, unsigned char* data, UINT alignment);

public:
	dx12buffermanager(dx12texturemanager* texture_manager);
	~dx12buffermanager();

	BufferResource CreateBuffer(void* data, unsigned int elementSize, unsigned int nrOfElements, const D3D12_RESOURCE_FLAGS& flags = D3D12_RESOURCE_FLAG_NONE, const D3D12_RESOURCE_STATES& initial_state = D3D12_RESOURCE_STATE_COMMON);
	BufferResource CreateBuffer(UINT buffer_size, const D3D12_RESOURCE_FLAGS& flags, const D3D12_RESOURCE_STATES& initial_state, const D3D12_HEAP_TYPE& heap_type = D3D12_HEAP_TYPE_DEFAULT);
	BufferResource CreateStructuredBuffer(void* data, unsigned int elementSize, unsigned int nrOfElements, TextureType texture_type);
	BufferResource CreateStructuredBuffer(UINT buffer_size, const D3D12_RESOURCE_FLAGS& flags, const D3D12_RESOURCE_STATES& initial_state, TextureType texture_type, const D3D12_HEAP_TYPE& heap_type = D3D12_HEAP_TYPE_DEFAULT);
	
	void UpdateBuffer(BufferResource buffer_resource, void* new_data);

	void ResetUploadBuffer();
};

