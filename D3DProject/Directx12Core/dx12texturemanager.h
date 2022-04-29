#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>
#include <vector>
#include <string>

enum class TextureType : UINT16
{
	TEXTURE_RTV = 1,
	TEXTURE_DSV = 2,
	TEXTURE_SRV = 4,
	TEXTURE_UAV = 8,
	TEXTURE_CBV = 16,
};

TextureType operator|(TextureType lhs, TextureType rhs);
UINT16 operator&(const TextureType& lhs, const TextureType& rhs);

struct dx12texture
{
	UINT descriptor_heap_offset;
	UINT resource_index;
	TextureType texture_type;
};

struct TextureResource
{
	dx12texture shader_resource_view;
	dx12texture render_target_view;
	dx12texture depth_stencil_view;
	dx12texture unordered_access_view;

	TextureResource();
};

struct TextureInfo
{
	unsigned char* texture_data;
	UINT width, height, comp, channels;
};
class dx12texturemanager
{
public:
	//Temp
	UINT m_acceleration_counter = 0;
private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dh_RTV;
	UINT m_dh_RTV_current_offset = 0;
	UINT m_max_dh_RTV_offset;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dh_DSV;
	UINT m_dh_DSV_current_offset = 0;
	UINT m_max_dh_DSV_offset;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dh_shader_bindable;
	UINT m_dh_shader_bindable_current_offset = 0;
	UINT m_max_dh_shader_bindable_offset;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_view_resources;

	//std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_render_target_view_resources;
	//std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_depth_stencil_view_resources;
	//std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_shader_bindable_resources;

	Microsoft::WRL::ComPtr<ID3D12Heap> m_upload_heap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_buffer;
	UINT64 m_upload_current_offset;

private:

	TextureInfo LoadTextureFromFile(const std::string& texture_file_name, UINT channels = 4) const;

	void UploadTextureData(ID3D12Resource* target_resource, unsigned char* data, UINT alignment);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateCommitedTexture(const TextureType& texture_type, const TextureInfo& texture_info, D3D12_CLEAR_VALUE* clear_value = nullptr);
	TextureResource CreateTextureViews(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const TextureType& texture_type);

public:
	dx12texturemanager(UINT max_render_target_views, UINT max_depth_stencil_views, UINT max_shader_bindables);
	~dx12texturemanager();

	TextureResource CreateRenderTargetView(UINT back_buffer_index);

	TextureResource CreateDepthStencilView(UINT texture_width, UINT texture_height, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	TextureResource CreateTexture2D(const std::string& texture_file_name, const TextureType& texture_type);
	TextureResource CreateTexture2D(UINT texture_width, UINT texture_height, const TextureType& texture_type, D3D12_CLEAR_VALUE* clear_value = nullptr);

	dx12texture CreateStructuredBuffer(ID3D12Resource* resource, UINT element_size, UINT nr_of_elements, TextureType texture_type);

	ID3D12Resource* GetTextureResource(UINT resource_index);

	ID3D12DescriptorHeap* GetShaderBindableDescriptorHeap();
	ID3D12DescriptorHeap* GetRenderTargetViewDescriptorHeap();
	ID3D12DescriptorHeap* GetDepthStencilViewDescriptorHeap();
};

