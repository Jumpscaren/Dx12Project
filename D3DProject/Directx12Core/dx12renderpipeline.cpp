#include "dx12renderpipeline.h"
#include "dx12core.h"
#include <fstream>
#include <d3dcompiler.h>

ID3DBlob* dx12renderpipeline::LoadCSO(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open())
		assert(false);
	file.seekg(0, std::ios_base::end);
	size_t size = static_cast<size_t>(file.tellg());
	file.seekg(0, std::ios_base::beg);
	ID3DBlob* cso_blob = nullptr;
	HRESULT hr = D3DCreateBlob(size, &cso_blob);
	if (FAILED(hr))
		assert(false);
	file.read(static_cast<char*>(cso_blob->GetBufferPointer()), size);
	file.close();
	return cso_blob;
}

void dx12renderpipeline::AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
{
	D3D12_DESCRIPTOR_RANGE descriptor_range;
	descriptor_range.RangeType = range_type;
	descriptor_range.NumDescriptors = 1;
	descriptor_range.BaseShaderRegister = binding_slot;
	descriptor_range.RegisterSpace = register_space;
	descriptor_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	std::vector<D3D12_DESCRIPTOR_RANGE> range = { descriptor_range };
	m_ranges.push_back(range);

	D3D12_ROOT_PARAMETER descriptor_table;
	descriptor_table.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	descriptor_table.ShaderVisibility = shader_type;
	descriptor_table.DescriptorTable.NumDescriptorRanges = m_ranges[m_ranges.size() - 1].size();
	descriptor_table.DescriptorTable.pDescriptorRanges = m_ranges[m_ranges.size() - 1].size() != 0 ? m_ranges[m_ranges.size() - 1].data() : nullptr;

	m_root_parameters.push_back(descriptor_table);
}

void dx12renderpipeline::CreateRootSignature(ID3D12RootSignature** root_signature, const D3D12_ROOT_SIGNATURE_FLAGS& flags)
{
	D3D12_ROOT_SIGNATURE_DESC desc;
	desc.NumParameters = static_cast<UINT>(m_root_parameters.size());
	desc.pParameters = m_root_parameters.size() == 0 ? nullptr : m_root_parameters.data();
	desc.NumStaticSamplers = static_cast<UINT>(m_static_samplers.size());
	desc.pStaticSamplers = m_static_samplers.size() == 0 ? nullptr : m_static_samplers.data();
	desc.Flags = flags;

	ID3DBlob* root_signature_blob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, nullptr);

	assert(SUCCEEDED(hr));

	hr = dx12core::GetDx12Core().GetDevice()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(),
		IID_PPV_ARGS(root_signature));
	assert(SUCCEEDED(hr));

	root_signature_blob->Release();
}

dx12renderpipeline::dx12renderpipeline()
{
}

dx12renderpipeline::~dx12renderpipeline()
{
	for (int i = 0; i < m_render_objects.size(); ++i)
	{
		delete m_render_objects[i];
	}
	m_render_objects.clear();
}

void dx12renderpipeline::AddConstantBuffer(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, D3D12_ROOT_PARAMETER_TYPE parameter_type, UINT register_space)
{
	RootRenderBinding new_root_binding;

	new_root_binding.root_parameter_index = m_root_parameters.size();

	new_root_binding.binding_slot = binding_slot;
	new_root_binding.shader_type = shader_type;
	new_root_binding.register_space = register_space;

	new_root_binding.binding_type = BindingType::CONSTANT_BUFFER;

	D3D12_ROOT_PARAMETER root_parameter;
	root_parameter.ParameterType = parameter_type;
	root_parameter.ShaderVisibility = shader_type;
	root_parameter.Descriptor.ShaderRegister = binding_slot;
	root_parameter.Descriptor.RegisterSpace = register_space;

	m_root_parameters.push_back(root_parameter);

	if (global)
		m_global_root_render_binding.push_back(new_root_binding);
	else
		m_object_root_render_binding.push_back(new_root_binding);
}

void dx12renderpipeline::AddStructuredBuffer(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
{
	RootRenderBinding new_root_binding;

	new_root_binding.root_parameter_index = m_root_parameters.size();

	new_root_binding.binding_slot = binding_slot;
	new_root_binding.shader_type = shader_type;
	new_root_binding.register_space = register_space;

	new_root_binding.binding_type = BindingType::STRUCTURED_BUFFER;

	AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, binding_slot, shader_type, global, register_space);

	if (global)
		m_global_root_render_binding.push_back(new_root_binding);
	else
		m_object_root_render_binding.push_back(new_root_binding);
}

void dx12renderpipeline::AddShaderResource(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
{
	RootRenderBinding new_root_binding;

	new_root_binding.root_parameter_index = m_root_parameters.size();

	new_root_binding.binding_slot = binding_slot;
	new_root_binding.shader_type = shader_type;
	new_root_binding.register_space = register_space;

	new_root_binding.binding_type = BindingType::SHADER_RESOURCE;

	AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, binding_slot, shader_type, global, register_space);

	if (global)
		m_global_root_render_binding.push_back(new_root_binding);
	else
		m_object_root_render_binding.push_back(new_root_binding);
}

void dx12renderpipeline::AddUnorderedAccess(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
{
	RootRenderBinding new_root_binding;

	new_root_binding.root_parameter_index = m_root_parameters.size();

	new_root_binding.binding_slot = binding_slot;
	new_root_binding.shader_type = shader_type;
	new_root_binding.register_space = register_space;

	new_root_binding.binding_type = BindingType::UNORDERED_ACCESS;

	AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, binding_slot, shader_type, global, register_space);

	if (global)
		m_global_root_render_binding.push_back(new_root_binding);
	else
		m_object_root_render_binding.push_back(new_root_binding);
}

void dx12renderpipeline::AddStaticSampler(D3D12_FILTER sampler_filter, D3D12_TEXTURE_ADDRESS_MODE sampler_address_mode, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
{
	D3D12_STATIC_SAMPLER_DESC sampler_desc;
	sampler_desc.MipLODBias = 0.0f;
	sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
	sampler_desc.Filter = sampler_filter;
	sampler_desc.AddressU = sampler_desc.AddressW = sampler_desc.AddressV = sampler_address_mode;

	sampler_desc.ShaderRegister = binding_slot;
	sampler_desc.RegisterSpace = register_space;
	sampler_desc.ShaderVisibility = shader_type;

	if (sampler_filter == D3D12_FILTER_ANISOTROPIC)
		sampler_desc.MaxAnisotropy = 16;
	else
		sampler_desc.MaxAnisotropy = 1;

	m_static_samplers.push_back(sampler_desc);
}

void dx12renderpipeline::CreateRenderPipeline(const std::string& vertex_shader, const std::string& pixel_shader)
{
	//Create root signature
	CreateRootSignature(m_root_signature.GetAddressOf());

	//Create pipeline
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc;
	ZeroMemory(&pipeline_desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	pipeline_desc.pRootSignature = m_root_signature.Get();

	D3D12_SHADER_BYTECODE* vs_shader = &pipeline_desc.VS;
	D3D12_SHADER_BYTECODE* ps_shader = &pipeline_desc.PS;
	ID3DBlob* vs_shader_blob = LoadCSO(vertex_shader);
	ID3DBlob* ps_shader_blob = LoadCSO(pixel_shader);
	vs_shader->pShaderBytecode = vs_shader_blob->GetBufferPointer();
	vs_shader->BytecodeLength = vs_shader_blob->GetBufferSize();
	ps_shader->pShaderBytecode = ps_shader_blob->GetBufferPointer();
	ps_shader->BytecodeLength = ps_shader_blob->GetBufferSize();

	pipeline_desc.SampleMask = UINT_MAX;

	D3D12_RASTERIZER_DESC rasterizer_desc;
	rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizer_desc.FrontCounterClockwise = false;
	rasterizer_desc.DepthBias = 0;
	rasterizer_desc.DepthBiasClamp = 0.0f;
	rasterizer_desc.SlopeScaledDepthBias = 0.0f;
	rasterizer_desc.DepthClipEnable = true;
	rasterizer_desc.MultisampleEnable = false;
	rasterizer_desc.AntialiasedLineEnable = false;
	rasterizer_desc.ForcedSampleCount = 0;
	rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	pipeline_desc.RasterizerState = rasterizer_desc;


	pipeline_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipeline_desc.NumRenderTargets = static_cast<UINT>(1);

	pipeline_desc.BlendState.AlphaToCoverageEnable = false;
	pipeline_desc.BlendState.IndependentBlendEnable = false;

	pipeline_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D12_RENDER_TARGET_BLEND_DESC rt_blend_desc;
	rt_blend_desc.BlendEnable = false;
	rt_blend_desc.LogicOpEnable = false;
	rt_blend_desc.SrcBlend = D3D12_BLEND_ONE;
	rt_blend_desc.DestBlend = D3D12_BLEND_ZERO;
	rt_blend_desc.BlendOp = D3D12_BLEND_OP_ADD;
	rt_blend_desc.SrcBlendAlpha = D3D12_BLEND_ONE;
	rt_blend_desc.DestBlendAlpha = D3D12_BLEND_ZERO;
	rt_blend_desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rt_blend_desc.LogicOp = D3D12_LOGIC_OP_NOOP;
	rt_blend_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	pipeline_desc.BlendState.RenderTarget[0] = rt_blend_desc;

	pipeline_desc.SampleDesc.Count = 1;
	pipeline_desc.SampleDesc.Quality = 0;

	D3D12_DEPTH_STENCIL_DESC depth_stencil_desc;
	depth_stencil_desc.DepthEnable = true;
	depth_stencil_desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depth_stencil_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depth_stencil_desc.StencilEnable = false;
	depth_stencil_desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	depth_stencil_desc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	depth_stencil_desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depth_stencil_desc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depth_stencil_desc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depth_stencil_desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depth_stencil_desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depth_stencil_desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	depth_stencil_desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_STREAM_OUTPUT_DESC stream_output_desc;
	stream_output_desc.pSODeclaration = nullptr;
	stream_output_desc.NumEntries = 0;
	stream_output_desc.pBufferStrides = nullptr;
	stream_output_desc.NumStrides = 0;
	stream_output_desc.RasterizedStream = 0;

	pipeline_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipeline_desc.DepthStencilState = depth_stencil_desc;
	pipeline_desc.StreamOutput = stream_output_desc;
	pipeline_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateGraphicsPipelineState(&pipeline_desc, IID_PPV_ARGS(m_pipeline_state.GetAddressOf()));
	assert(SUCCEEDED(hr));

	ps_shader_blob->Release();
	vs_shader_blob->Release();

	m_root_parameters.clear();
	m_ranges.clear();
}

ID3D12RootSignature* dx12renderpipeline::GetRootSignature()
{
	return m_root_signature.Get();
}

ID3D12PipelineState* dx12renderpipeline::GetPipelineState()
{
	return m_pipeline_state.Get();
}

std::vector<RootRenderBinding>* dx12renderpipeline::GetObjectRootBinds()
{
	return &m_object_root_render_binding;
}

std::vector<RootRenderBinding>* dx12renderpipeline::GetGlobalRootBinds()
{
	return &m_global_root_render_binding;
}

std::vector<RenderObject*>& dx12renderpipeline::GetObjects()
{
	return m_render_objects;
}

RenderObject* dx12renderpipeline::CreateRenderObject()
{
	RenderObject* render_object = new RenderObject();
	render_object->SetRenderPipeline(this);
	m_render_objects.push_back(render_object);
	return render_object;
}

void dx12renderpipeline::CreateRayTracingStateObject(const std::string& shader_name)
{
	D3D12_STATE_SUBOBJECT stateSubobjects[6];

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
	shaderConfig.MaxPayloadSizeInBytes = sizeof(float) * 3;
	shaderConfig.MaxAttributeSizeInBytes = sizeof(float) * 2;
	stateSubobjects[0].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	stateSubobjects[0].pDesc = &shaderConfig;

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = 1;
	stateSubobjects[1].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	stateSubobjects[1].pDesc = &pipelineConfig;

	D3D12_LOCAL_ROOT_SIGNATURE localRootSignature;
	CreateRootSignature(m_raytracing_root_signature.GetAddressOf(), D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	localRootSignature.pLocalRootSignature = m_raytracing_root_signature.Get();
	stateSubobjects[2].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	stateSubobjects[2].pDesc = &localRootSignature;

	D3D12_DXIL_LIBRARY_DESC dxilLibrary;
	ID3DBlob* libraryBlob = LoadCSO(shader_name);
	dxilLibrary.DXILLibrary.pShaderBytecode = libraryBlob->GetBufferPointer();
	dxilLibrary.DXILLibrary.BytecodeLength = libraryBlob->GetBufferSize();
	dxilLibrary.NumExports = 0;
	dxilLibrary.pExports = nullptr;
	stateSubobjects[3].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	stateSubobjects[3].pDesc = &dxilLibrary;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exportsAssociation;
	exportsAssociation.pSubobjectToAssociate = &stateSubobjects[2]; //LOCAL ROOT SIGNATURE
	exportsAssociation.NumExports = 0;
	exportsAssociation.pExports = nullptr;
	stateSubobjects[4].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	stateSubobjects[4].pDesc = &exportsAssociation;

	D3D12_HIT_GROUP_DESC hitGroupDesc;
	hitGroupDesc.HitGroupExport = L"HitGroup";
	hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupDesc.AnyHitShaderImport = nullptr;
	hitGroupDesc.ClosestHitShaderImport = L"ClosestHitShader";
	hitGroupDesc.IntersectionShaderImport = nullptr;
	stateSubobjects[5].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	stateSubobjects[5].pDesc = &hitGroupDesc;

	D3D12_STATE_OBJECT_DESC description;
	description.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	description.NumSubobjects = ARRAYSIZE(stateSubobjects);
	description.pSubobjects = stateSubobjects;

	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateStateObject(&description, IID_PPV_ARGS(m_raytracing_state_object.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void dx12renderpipeline::CreateShaderRecordBuffers()
{
	const int ROOT_ARGUMENT_SIZE = 32;
	unsigned char root_argument_data[ROOT_ARGUMENT_SIZE];

	auto root_arg_1 = dx12core::GetDx12Core().GetTopLevelResultAccelerationStructureBuffer()->GetGPUVirtualAddress();
	D3D12_CPU_DESCRIPTOR_HANDLE uav_handle = dx12core::GetDx12Core().GetTextureManager()->GetShaderBindableDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	UINT shader_bindable_size = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	uav_handle.ptr += dx12core::GetDx12Core().GetOutputUAV().unordered_access_view.descriptor_heap_offset * shader_bindable_size;
	auto root_arg_2 = uav_handle;
	memcpy(root_argument_data, &root_arg_1, sizeof(root_arg_1));
	memcpy(root_argument_data + sizeof(root_arg_1), &root_arg_2, sizeof(root_arg_2));

	//Ray Gen
	ShaderRecord<ROOT_ARGUMENT_SIZE> ray_gen_record_data;

	CreateShaderRecord(ray_gen_record_data, L"RayGenerationShader", root_argument_data);

	ray_gen_record.buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&ray_gen_record_data), sizeof(ray_gen_record_data), 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(ray_gen_record.buffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//Miss
	ShaderRecord<ROOT_ARGUMENT_SIZE> miss_record_data;
	CreateShaderRecord(miss_record_data, L"MissShader", root_argument_data);

	miss_record.buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&miss_record_data), sizeof(miss_record_data), 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(miss_record.buffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//Hit
	ShaderRecord<ROOT_ARGUMENT_SIZE> hit_record_data;
	CreateShaderRecord(hit_record_data, L"HitGroup", root_argument_data);

	hit_record.buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&hit_record_data), sizeof(hit_record_data), 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(hit_record.buffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

RootRenderBinding* RenderObject::AddResource(BindingType binding_type, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
{
	if (!render_pipeline)
		assert(false);

	dx12renderpipeline* render_pipeline_ptr = (dx12renderpipeline*)render_pipeline;
	RootRenderBinding* root_binding = nullptr;
	if (global)
	{
		//auto bindings = render_pipeline_ptr->GetGlobalRootBinds();
		//for (auto bind : bindings)
		//{
		//	if (bind.binding_type == binding_type && bind.binding_slot == binding_slot && bind.shader_type == shader_type && bind.register_space == register_space)
		//	{
		//		root_binding = &bind;
		//		break;
		//	}
		//}
	}
	else
	{
		auto bindings = render_pipeline_ptr->GetObjectRootBinds();
		for (int i = 0; i < bindings->size(); ++i)
		{
			auto bind = (*bindings)[i];
			if (bind.binding_type == binding_type && bind.binding_slot == binding_slot && bind.shader_type == shader_type && bind.register_space == register_space)
			{
				root_binding = &(*bindings)[i];
				break;
			}
		}
	}

	assert(root_binding != nullptr);

	return root_binding;
}

void RenderObject::AddStructuredBuffer(const BufferResource& buffer_resource, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
{
	std::pair<BufferResource, RootRenderBinding*> buffer;
	buffer.first = buffer_resource;
	buffer.second = AddResource(BindingType::STRUCTURED_BUFFER, binding_slot, shader_type, global, register_space);
	buffers.push_back(buffer);
}

void RenderObject::AddShaderResourceView(const TextureResource& texture_resource, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
{
	std::pair<TextureResource, RootRenderBinding*> texture;
	texture.first = texture_resource;
	texture.second = AddResource(BindingType::SHADER_RESOURCE, binding_slot, shader_type, global, register_space);
	textures.push_back(texture);
}

void RenderObject::SetRenderPipeline(void* render_pipeline_in)
{
	render_pipeline = render_pipeline_in;
}

void RenderObject::SetMeshData(UINT element_size, UINT nr_of_elements)
{
	mesh_element_size = element_size;
	mesh_nr_of_elements = nr_of_elements;
}

std::vector<std::pair<BufferResource, RootRenderBinding*>>& RenderObject::GetBuffers()
{
	return buffers;
}

std::vector<std::pair<TextureResource, RootRenderBinding*>>& RenderObject::GetTextures()
{
	return textures;
}

RenderObject::~RenderObject()
{
}
