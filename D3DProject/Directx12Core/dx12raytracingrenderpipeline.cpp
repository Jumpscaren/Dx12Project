#include "dx12raytracingrenderpipeline.h"
#include "dx12core.h"
#include <fstream>
#include <d3dcompiler.h>

dx12raytracingrenderpipeline::dx12raytracingrenderpipeline()
{
}

dx12raytracingrenderpipeline::~dx12raytracingrenderpipeline()
{
}

void dx12raytracingrenderpipeline::CreateRayTracingStateObject(const std::string& shader_name, const std::wstring& hit_shader_name, UINT payload_size, UINT max_bounces)
{
	D3D12_STATE_SUBOBJECT stateSubobjects[7];

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
	shaderConfig.MaxPayloadSizeInBytes = payload_size;

	// makes it barycentric coordinates
	shaderConfig.MaxAttributeSizeInBytes = sizeof(float) * 2;
	stateSubobjects[0].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	stateSubobjects[0].pDesc = &shaderConfig;

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = max_bounces + 1;
	stateSubobjects[1].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	stateSubobjects[1].pDesc = &pipelineConfig;

	D3D12_LOCAL_ROOT_SIGNATURE localRootSignature;
	CreateRootSignature(m_raytracing_local_root_signature.GetAddressOf(), false);

	localRootSignature.pLocalRootSignature = m_raytracing_local_root_signature.Get();
	stateSubobjects[2].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	stateSubobjects[2].pDesc = &localRootSignature;

	D3D12_GLOBAL_ROOT_SIGNATURE global_root_signature;
	CreateRootSignature(m_raytracing_global_root_signature.GetAddressOf(), true);// D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	global_root_signature.pGlobalRootSignature = m_raytracing_global_root_signature.Get();
	stateSubobjects[3].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;// D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	stateSubobjects[3].pDesc = &global_root_signature;

	D3D12_DXIL_LIBRARY_DESC dxilLibrary;
	ID3DBlob* libraryBlob = LoadCSO(shader_name);
	dxilLibrary.DXILLibrary.pShaderBytecode = libraryBlob->GetBufferPointer();
	dxilLibrary.DXILLibrary.BytecodeLength = libraryBlob->GetBufferSize();
	dxilLibrary.NumExports = 0;
	dxilLibrary.pExports = nullptr;
	stateSubobjects[4].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	stateSubobjects[4].pDesc = &dxilLibrary;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exportsAssociation;
	exportsAssociation.pSubobjectToAssociate = &stateSubobjects[2]; //LOCAL ROOT SIGNATURE
	exportsAssociation.NumExports = 0;
	exportsAssociation.pExports = nullptr;
	stateSubobjects[5].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	stateSubobjects[5].pDesc = &exportsAssociation;

	D3D12_HIT_GROUP_DESC hitGroupDesc;
	hitGroupDesc.HitGroupExport = L"HitGroup";
	hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupDesc.AnyHitShaderImport = nullptr;
	hitGroupDesc.ClosestHitShaderImport = hit_shader_name.c_str();//L"ClosestHitShader";
	hitGroupDesc.IntersectionShaderImport = nullptr;
	stateSubobjects[6].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	stateSubobjects[6].pDesc = &hitGroupDesc;

	D3D12_STATE_OBJECT_DESC description;
	description.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	description.NumSubobjects = ARRAYSIZE(stateSubobjects);
	description.pSubobjects = stateSubobjects;

	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateStateObject(&description, IID_PPV_ARGS(m_raytracing_state_object.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void dx12raytracingrenderpipeline::CreateShaderRecordBuffers(const std::wstring& ray_generation_shader_name, const std::wstring& miss_shader_name)
{
	const int ROOT_ARGUMENT_SIZE = 32;
	unsigned char root_argument_data[ROOT_ARGUMENT_SIZE];

	UINT shader_bindable_size = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	BufferResource top_level_structure = dx12core::GetDx12Core().GetRayObjectManager()->GetTopLevelResultAccelerationStructureBuffer();

	D3D12_GPU_DESCRIPTOR_HANDLE top_level_structures_handle = dx12core::GetDx12Core().GetTextureManager()->GetShaderBindableDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	top_level_structures_handle.ptr += top_level_structure.structured_buffer.descriptor_heap_offset * shader_bindable_size;
	auto root_arg_1 = top_level_structures_handle;
	// 
	//auto root_arg_1 = dx12core::GetDx12Core().GetRayObjectManager()->GetTopLevelResultAccelerationStructureBuffer()->GetGPUVirtualAddress();
	//auto root_arg_1 = dx12core::GetDx12Core().GetRayObjectManager()->GetTopLevelInstanceBuffer()->GetGPUVirtualAddress();

	D3D12_GPU_DESCRIPTOR_HANDLE uav_handle = dx12core::GetDx12Core().GetTextureManager()->GetShaderBindableDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	uav_handle.ptr += dx12core::GetDx12Core().GetOutputUAV().unordered_access_view.descriptor_heap_offset * shader_bindable_size;
	auto root_arg_2 = uav_handle;

	memcpy(root_argument_data, &root_arg_1, sizeof(root_arg_1));
	memcpy(root_argument_data + sizeof(root_arg_1), &root_arg_2, sizeof(root_arg_2));

	//Ray Gen
	ShaderRecordF<ROOT_ARGUMENT_SIZE> ray_gen_record_data;

	CreateShaderRecord(ray_gen_record_data, ray_generation_shader_name.c_str(), root_argument_data);

	m_ray_gen_record.buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&ray_gen_record_data), sizeof(ray_gen_record_data), 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(m_ray_gen_record.buffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//Miss
	ShaderRecordF<ROOT_ARGUMENT_SIZE> miss_record_data;
	CreateShaderRecord(miss_record_data, miss_shader_name.c_str(), root_argument_data);

	m_miss_record.buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&miss_record_data), sizeof(miss_record_data), 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(m_miss_record.buffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	//Hit
	ShaderRecordF<ROOT_ARGUMENT_SIZE> hit_record_data;
	CreateShaderRecord(hit_record_data, L"HitGroup", root_argument_data);

	m_hit_record.buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&hit_record_data), sizeof(hit_record_data), 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(m_hit_record.buffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void dx12raytracingrenderpipeline::CheckIfRaytracingRenderPipeline()
{
	assert(m_raytracing_state_object.Get());
}

ID3D12StateObject* dx12raytracingrenderpipeline::GetRaytracingStateObject()
{
	return m_raytracing_state_object.Get();
}

ID3D12RootSignature* dx12raytracingrenderpipeline::GetRaytracingLocalRootSignature()
{
	return m_raytracing_local_root_signature.Get();
}

ID3D12RootSignature* dx12raytracingrenderpipeline::GetRaytracingGlobalRootSignature()
{
	return m_raytracing_global_root_signature.Get();
}

RayGenerationShaderRecordF<32>* dx12raytracingrenderpipeline::RayGenerationShaderRecord()
{
	return &m_ray_gen_record;
}

ShaderTableF<32, 1>* dx12raytracingrenderpipeline::GetMissShaderRecord()
{
	return &m_miss_record;
}

ShaderTableF<32, 1>* dx12raytracingrenderpipeline::GetHitShaderRecord()
{
	return &m_hit_record;
}

ID3DBlob* dx12raytracingrenderpipeline::LoadCSO(const std::string& filepath)
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

void dx12raytracingrenderpipeline::AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT descriptors, UINT register_space)
{
	D3D12_DESCRIPTOR_RANGE descriptor_range;
	descriptor_range.RangeType = range_type;
	descriptor_range.NumDescriptors = descriptors;
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

	if (global)
		m_global_root_parameters.push_back(descriptor_table);
	else
		m_local_root_parameters.push_back(descriptor_table);
}

void dx12raytracingrenderpipeline::CreateRootSignature(ID3D12RootSignature** root_signature, bool global)
{
	D3D12_ROOT_SIGNATURE_DESC desc;
	if (global == false)
	{
		desc.NumParameters = static_cast<UINT>(m_local_root_parameters.size());
		desc.pParameters = m_local_root_parameters.size() == 0 ? nullptr : m_local_root_parameters.data();
		desc.NumStaticSamplers = static_cast<UINT>(m_static_samplers.size());
		desc.pStaticSamplers = m_static_samplers.size() == 0 ? nullptr : m_static_samplers.data();
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE; //flags;
	}
	else
	{
		desc.NumParameters = static_cast<UINT>(m_global_root_parameters.size());
		desc.pParameters = m_global_root_parameters.size() == 0 ? nullptr : m_global_root_parameters.data();
		desc.NumStaticSamplers = static_cast<UINT>(m_static_samplers.size());
		desc.pStaticSamplers = m_static_samplers.size() == 0 ? nullptr : m_static_samplers.data();
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; //flags;
	}

	ID3DBlob* root_signature_blob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, nullptr);

	assert(SUCCEEDED(hr));

	hr = dx12core::GetDx12Core().GetDevice()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(),
		IID_PPV_ARGS(root_signature));
	assert(SUCCEEDED(hr));

	root_signature_blob->Release();
}

void dx12raytracingrenderpipeline::AddConstantBuffer(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, D3D12_ROOT_PARAMETER_TYPE parameter_type, UINT register_space)
{
	D3D12_ROOT_PARAMETER root_parameter;
	root_parameter.ParameterType = parameter_type;
	root_parameter.ShaderVisibility = shader_type;
	root_parameter.Descriptor.ShaderRegister = binding_slot;
	root_parameter.Descriptor.RegisterSpace = register_space;

	if (global)
		m_global_root_parameters.push_back(root_parameter);
	else
		m_local_root_parameters.push_back(root_parameter);
}

void dx12raytracingrenderpipeline::AddStructuredBuffer(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT register_space)
{
	AddDescriptorTable(range_type, binding_slot, shader_type, global, 1, register_space);
}

void dx12raytracingrenderpipeline::AddShaderResource(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT descriptors, UINT register_space)
{
	AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, binding_slot, shader_type, global, descriptors, register_space);
}

void dx12raytracingrenderpipeline::AddUnorderedAccess(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
{
	AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, binding_slot, shader_type, global, 1, register_space);
}

void dx12raytracingrenderpipeline::AddStaticSampler(D3D12_FILTER sampler_filter, D3D12_TEXTURE_ADDRESS_MODE sampler_address_mode, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space)
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