#pragma once
#include "dx12texturemanager.h"

struct BufferResource
{
	Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
	dx12texture structured_buffer;

	BufferResource();
};

class dx12buffermanager
{
private:
	dx12texturemanager* m_texture_manager;

	Microsoft::WRL::ComPtr<ID3D12Heap> m_upload_heap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_buffer;
	UINT64 m_upload_current_offset;

	void UploadBufferData(ID3D12Resource* upload_resource, unsigned char* data, UINT alignment);

public:
	dx12buffermanager(dx12texturemanager* texture_manager);

	BufferResource CreateBuffer(void* data, unsigned int elementSize, unsigned int nrOfElements);
	BufferResource CreateStructuredBuffer(void* data, unsigned int elementSize, unsigned int nrOfElements);
};

