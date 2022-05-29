#include "dx12raytracingrenderpipeline.h"
#include "dx12core.h"
#include <fstream>
#include <d3dcompiler.h>

dx12raytracingrenderpipeline::dx12raytracingrenderpipeline()
{
}

dx12raytracingrenderpipeline::~dx12raytracingrenderpipeline()
{
	m_miss_record.DeleteData();
	m_hit_record.DeleteData();
	m_ray_gen_record.DeleteData();
}

void dx12raytracingrenderpipeline::CreateRayTracingStateObject(const std::string& shader_library_name, std::vector<HitGroupInfo>& hit_groups, UINT payload_size, UINT attribute_size, UINT max_bounces)
{
	const int base_state_subobjects_amount = 5;

	std::vector<D3D12_STATE_SUBOBJECT> state_subobjects(base_state_subobjects_amount + hit_groups.size());

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
	shaderConfig.MaxPayloadSizeInBytes = payload_size;

	shaderConfig.MaxAttributeSizeInBytes = attribute_size;
	state_subobjects[0].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	state_subobjects[0].pDesc = &shaderConfig;

	D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config;
	pipeline_config.MaxTraceRecursionDepth = max_bounces + 1;
	state_subobjects[1].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	state_subobjects[1].pDesc = &pipeline_config;

	D3D12_LOCAL_ROOT_SIGNATURE local_root_signature;
	CreateRootSignature(m_raytracing_local_root_signature.GetAddressOf(), false);

	local_root_signature.pLocalRootSignature = m_raytracing_local_root_signature.Get();
	state_subobjects[2].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	state_subobjects[2].pDesc = &local_root_signature;

	D3D12_DXIL_LIBRARY_DESC dxil_library;
	ID3DBlob* libraryBlob = LoadCSO(shader_library_name);
	dxil_library.DXILLibrary.pShaderBytecode = libraryBlob->GetBufferPointer();
	dxil_library.DXILLibrary.BytecodeLength = libraryBlob->GetBufferSize();
	dxil_library.NumExports = 0;
	dxil_library.pExports = nullptr;
	state_subobjects[3].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	state_subobjects[3].pDesc = &dxil_library;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exports_association;
	exports_association.pSubobjectToAssociate = &state_subobjects[2];
	exports_association.NumExports = 0;
	exports_association.pExports = nullptr;
	state_subobjects[4].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	state_subobjects[4].pDesc = &exports_association;

	std::vector<D3D12_HIT_GROUP_DESC> hit_group_descs(hit_groups.size());

	for (int i = 0; i < hit_groups.size(); ++i)
	{
		SetHitGroup(hit_groups[i], hit_group_descs[i]);
		state_subobjects[base_state_subobjects_amount + i].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		state_subobjects[base_state_subobjects_amount + i].pDesc = &hit_group_descs[i];
	}

	D3D12_STATE_OBJECT_DESC description;
	description.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	description.NumSubobjects = state_subobjects.size();
	description.pSubobjects = state_subobjects.data();

	HRESULT hr = dx12core::GetDx12Core().GetDevice()->CreateStateObject(&description, IID_PPV_ARGS(m_raytracing_state_object.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void dx12raytracingrenderpipeline::CreateShaderRecordBuffers(const std::wstring& ray_generation_shader_name, const std::wstring& miss_shader_name, BufferResource triangle_colours
	, BufferResource view_projection_matrix, BufferResource sphere_positions, std::vector<HitGroupInfo>& hit_groups)
{
	const int ROOT_ARGUMENT_SIZE = 64;
	unsigned char root_argument_data[ROOT_ARGUMENT_SIZE];

	UINT shader_bindable_size = dx12core::GetDx12Core().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	BufferResource top_level_structure = dx12core::GetDx12Core().GetRayObjectManager()->GetTopLevelResultAccelerationStructureBuffer();

	D3D12_GPU_DESCRIPTOR_HANDLE top_level_structures_handle = dx12core::GetDx12Core().GetTextureManager()->GetShaderBindableDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	top_level_structures_handle.ptr += top_level_structure.structured_buffer.descriptor_heap_offset * shader_bindable_size;
	auto root_arg_1 = top_level_structures_handle;

	D3D12_GPU_DESCRIPTOR_HANDLE uav_handle = dx12core::GetDx12Core().GetTextureManager()->GetShaderBindableDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	uav_handle.ptr += dx12core::GetDx12Core().GetOutputUAV().unordered_access_view.descriptor_heap_offset * shader_bindable_size;
	auto root_arg_2 = uav_handle;

	D3D12_GPU_DESCRIPTOR_HANDLE triangle_colours_handle = dx12core::GetDx12Core().GetTextureManager()->GetShaderBindableDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	triangle_colours_handle.ptr += triangle_colours.structured_buffer.descriptor_heap_offset * shader_bindable_size;
	auto root_arg_3 = triangle_colours_handle;

	auto root_arg_4 = view_projection_matrix.buffer->GetGPUVirtualAddress();

	D3D12_GPU_DESCRIPTOR_HANDLE sphere_positions_handle = dx12core::GetDx12Core().GetTextureManager()->GetShaderBindableDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	sphere_positions_handle.ptr += sphere_positions.structured_buffer.descriptor_heap_offset * shader_bindable_size;
	auto root_arg_5 = sphere_positions_handle;

	D3D12_GPU_DESCRIPTOR_HANDLE data_indices_handle = dx12core::GetDx12Core().GetTextureManager()->GetShaderBindableDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	data_indices_handle.ptr += dx12core::GetDx12Core().GetRayObjectManager()->GetDataIndices().structured_buffer.descriptor_heap_offset * shader_bindable_size;
	auto root_arg_6 = data_indices_handle;

	int offset = 0;
	memcpy(root_argument_data, &root_arg_1, sizeof(root_arg_1));
	offset += sizeof(root_arg_1);
	memcpy(root_argument_data + offset, &root_arg_2, sizeof(root_arg_2));
	offset += sizeof(root_arg_2);
	memcpy(root_argument_data + offset, &root_arg_3, sizeof(root_arg_3));
	offset += sizeof(root_arg_3);
	memcpy(root_argument_data + offset, &root_arg_4, sizeof(root_arg_4));
	offset += sizeof(root_arg_4);
	memcpy(root_argument_data + offset, &root_arg_5, sizeof(root_arg_5));
	offset += sizeof(root_arg_5);
	memcpy(root_argument_data + offset, &root_arg_6, sizeof(root_arg_6));
	offset += sizeof(root_arg_6);

	//Ray Gen
	RayShaderRecord ray_gen_record_data = CreateShaderRecord(ROOT_ARGUMENT_SIZE, ray_generation_shader_name.c_str(), root_argument_data);

	m_ray_gen_record.CopyShaderRecordData({ray_gen_record_data});

	m_ray_gen_record.buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(ray_gen_record_data.shader_record_data), ray_gen_record_data.shader_record_size, 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(m_ray_gen_record.buffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	ray_gen_record_data.DeleteData();


	//Miss shader

	RayShaderRecord miss_record_data = CreateShaderRecord(ROOT_ARGUMENT_SIZE, miss_shader_name.c_str(), root_argument_data);

	m_miss_record.CopyShaderRecordData({miss_record_data});

	m_miss_record.buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(miss_record_data.shader_record_data), miss_record_data.shader_record_size, 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(m_miss_record.buffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	miss_record_data.DeleteData();

	//Hit Groups

	std::vector<RayShaderRecord> hit_shader_records;
	for (int i = 0; i < hit_groups.size(); ++i)
	{
		RayShaderRecord hit_record_data = CreateShaderRecord(ROOT_ARGUMENT_SIZE, hit_groups[i].hit_group_name.c_str(), root_argument_data);
		hit_shader_records.push_back(hit_record_data);
	}

	RayShaderRecordTable hit_records;
	hit_records.CopyShaderRecordData(hit_shader_records);

	m_hit_record = hit_records;
	m_hit_record.buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(hit_records.multiple_shader_record_data), hit_records.entire_shader_record_size, 1, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(m_hit_record.buffer.buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	for (int i = 0; i < hit_shader_records.size(); ++i)
	{
		hit_shader_records[i].DeleteData();
	}
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

RayShaderRecordTable* dx12raytracingrenderpipeline::RayGenerationShaderRecord()
{
	return &m_ray_gen_record;
}

RayShaderRecordTable* dx12raytracingrenderpipeline::GetMissShaderRecord()
{
	return &m_miss_record;
}

RayShaderRecordTable* dx12raytracingrenderpipeline::GetHitShaderRecord()
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
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE; 
	}
	else
	{
		desc.NumParameters = static_cast<UINT>(m_global_root_parameters.size());
		desc.pParameters = m_global_root_parameters.size() == 0 ? nullptr : m_global_root_parameters.data();
		desc.NumStaticSamplers = static_cast<UINT>(m_static_samplers.size());
		desc.pStaticSamplers = m_static_samplers.size() == 0 ? nullptr : m_static_samplers.data();
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; 
	}

	ID3DBlob* root_signature_blob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, nullptr);

	assert(SUCCEEDED(hr));

	hr = dx12core::GetDx12Core().GetDevice()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(), root_signature_blob->GetBufferSize(),
		IID_PPV_ARGS(root_signature));
	assert(SUCCEEDED(hr));

	root_signature_blob->Release();
}

RayShaderRecord dx12raytracingrenderpipeline::CreateShaderRecord(UINT root_argument_size, const wchar_t* shader_name, void* root_argument_data)
{
	unsigned char* data = new unsigned char[root_argument_size + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];

	ID3D12StateObjectProperties* stateObjectProperties = nullptr;
	HRESULT hr = m_raytracing_state_object->QueryInterface(IID_PPV_ARGS(&stateObjectProperties));
	assert(SUCCEEDED(hr));

	memcpy(data, stateObjectProperties->GetShaderIdentifier(shader_name), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	memcpy(data + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, root_argument_data, root_argument_size);

	stateObjectProperties->Release();

	return {root_argument_size, root_argument_size + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, data};
}

void dx12raytracingrenderpipeline::SetHitGroup(const HitGroupInfo& hit_group_info, D3D12_HIT_GROUP_DESC& hit_group_desc)
{
	hit_group_desc.HitGroupExport = hit_group_info.hit_group_name.c_str();
	hit_group_desc.Type = hit_group_info.hit_type;
	hit_group_desc.AnyHitShaderImport = hit_group_info.any_hit_shader_name == L"" ? nullptr : hit_group_info.any_hit_shader_name.c_str();
	hit_group_desc.ClosestHitShaderImport = hit_group_info.closest_hit_shader_name == L"" ? nullptr : hit_group_info.closest_hit_shader_name.c_str();
	hit_group_desc.IntersectionShaderImport = hit_group_info.intersection_shader_name == L"" ? nullptr : hit_group_info.intersection_shader_name.c_str();
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