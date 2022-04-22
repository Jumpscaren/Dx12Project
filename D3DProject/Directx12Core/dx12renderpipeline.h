#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include "dx12buffermanager.h"

enum class BindingType
{
	CONSTANT_BUFFER = 1,
	STRUCTURED_BUFFER = 2,
	SHADER_RESOURCE = 4,
	UNORDERED_ACCESS = 8,
	SAMPLER = 16
};

struct RootRenderBinding
{
	UINT root_parameter_index;
	UINT binding_slot;
	D3D12_SHADER_VISIBILITY shader_type;
	UINT register_space;
	BindingType binding_type;
};

struct RenderObject
{
private:
	std::vector<std::pair<BufferResource, RootRenderBinding*>> buffers;
	std::vector<std::pair<TextureResource, RootRenderBinding*>> textures;
	void* render_pipeline = nullptr;
	RootRenderBinding* AddResource(BindingType binding_type, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space);
public:
	void AddStructuredBuffer(const BufferResource& buffer_resource, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);
	void AddShaderResourceView(const TextureResource& texture_resource, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);
	void SetRenderPipeline(void* render_pipeline_in);

	std::vector<std::pair<BufferResource, RootRenderBinding*>>& GetBuffers();
	std::vector<std::pair<TextureResource, RootRenderBinding*>>& GetTextures();;
};

class dx12renderpipeline
{
private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline_state;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature;

	std::vector<D3D12_ROOT_PARAMETER> m_root_parameters;
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> m_ranges;

	std::vector<RootRenderBinding> m_global_root_render_binding;
	std::vector<RootRenderBinding> m_object_root_render_binding;

	std::vector<D3D12_STATIC_SAMPLER_DESC> m_static_samplers;

	std::vector<RenderObject*> m_render_objects;

private:
	ID3DBlob* LoadCSO(const std::string& filepath);
	void AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);

public:
	dx12renderpipeline();
	~dx12renderpipeline();
	void AddConstantBuffer(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);
	void AddStructuredBuffer(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);
	void AddShaderResource(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);
	void AddUnorderedAccess(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);
	void AddStaticSampler(D3D12_FILTER sampler_filter, D3D12_TEXTURE_ADDRESS_MODE sampler_address_mode, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);
	void CreateRenderPipeline(const std::string& vertex_shader, const std::string& pixel_shader);
	ID3D12RootSignature* GetRootSignature();
	ID3D12PipelineState* GetPipelineState();
	std::vector<RootRenderBinding>* GetObjectRootBinds();
	std::vector<RootRenderBinding>* GetGlobalRootBinds();
	std::vector<RenderObject*>& GetObjects();

	RenderObject* CreateRenderObject();
};

