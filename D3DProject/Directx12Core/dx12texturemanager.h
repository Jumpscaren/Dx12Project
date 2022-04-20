#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>
#include <vector>

enum class TextureType
{
	TEXTURE_RTV = 0,
	TEXTURE_DSV = 1,
	TEXTURE_SRV = 2,
	TEXTURE_UAV = 3,
};

struct dx12texture
{
	UINT descriptor_heap_offset;
	UINT resource_index;
	TextureType texture_type;
};

class dx12texturemanager
{
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

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_render_target_view_resources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_depth_stencil_view_resources;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_shader_bindable_resources;
public:
	dx12texturemanager(UINT max_render_target_views, UINT max_depth_stencil_views, UINT max_shader_bindables);
	~dx12texturemanager();

	dx12texture CreateRenderTargetView(UINT back_buffer_index);

	dx12texture CreateDepthStencilView(UINT texture_width, UINT texture_height, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
};

