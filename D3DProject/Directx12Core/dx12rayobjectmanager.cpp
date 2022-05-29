#include "dx12rayobjectmanager.h"
#include "dx12core.h"

dx12rayobjectmanager::dx12rayobjectmanager()
{
}

dx12rayobjectmanager::~dx12rayobjectmanager()
{
	m_bottom_level_acceleration_structures.clear();
	m_top_level_acceleration_structures.clear();
	m_meshes.clear();
	m_aabbs.clear();
}

void dx12rayobjectmanager::AddMesh(BufferResource mesh_buffer, BufferResource transform)
{
	AddedMesh mesh = { mesh_buffer, transform };
	m_meshes.push_back(mesh);
}

void dx12rayobjectmanager::AddAABB(BufferResource aabb_buffer)
{
	m_aabbs.push_back({aabb_buffer});
}

RayTracingObject dx12rayobjectmanager::CreateRayTracingObject(UINT hit_shader_index, DirectX::XMFLOAT3X4 instance_transform, UINT data_index)
{
	if (m_meshes.size() == 0 || m_aabbs.size() > 0)
		assert(false);

	UINT bottom_level_index = BuildBottomLevelAcceleratonStructure(instance_transform, hit_shader_index);

	RayTracingObject object = { bottom_level_index, data_index, instance_transform };

	m_meshes.clear();

	return object;
}

RayTracingObject dx12rayobjectmanager::CreateRayTracingObjectAABB(UINT hit_shader_index, DirectX::XMFLOAT3X4 instance_transform, UINT data_index)
{
	if (m_aabbs.size() == 0 || m_meshes.size() > 0)
		assert(false);

	UINT bottom_level_index = BuildBottomLevelAcceleratonStructureAABB(instance_transform, hit_shader_index);

	RayTracingObject object = { bottom_level_index, data_index, instance_transform };

	m_aabbs.clear();

	return object;
}

RayTracingObject dx12rayobjectmanager::CopyRayTracingObjectAABB(RayTracingObject& ray_tracing_object, DirectX::XMFLOAT3X4 instance_transform, UINT data_index)
{
	return {ray_tracing_object.bottom_level_index, data_index, instance_transform};
}

void dx12rayobjectmanager::CreateScene(const std::vector<RayTracingObject>& objects)
{
	BuildTopLevelAccelerationStructure(objects);
	std::vector<UINT32> indices(objects.size());
	for (int i = 0; i < indices.size(); ++i)
	{
		indices[i] = objects[i].data_index;
	}
	m_data_indices = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(indices.data(), sizeof(UINT32), indices.size(), TextureType::TEXTURE_SRV);
}

void dx12rayobjectmanager::UpdateScene(const std::vector<RayTracingObject>& objects)
{
	UpdateTopLevelAccelerationStructure(objects, 0);
}

const BufferResource& dx12rayobjectmanager::GetTopLevelResultAccelerationStructureBuffer(UINT top_level_index)
{
	return m_top_level_acceleration_structures[top_level_index].result_buffer;
}

const BufferResource& dx12rayobjectmanager::GetTopLevelScratchAccelerationStructureBuffer(UINT top_level_index)
{
	return m_top_level_acceleration_structures[top_level_index].scratch_buffer;
}

const BufferResource& dx12rayobjectmanager::GetTopLevelInstanceBuffer(UINT top_level_index)
{
	return m_top_level_acceleration_structures[top_level_index].instance_buffer;
}

const BufferResource& dx12rayobjectmanager::GetBottomLevelScratchAccelerationStructureBuffer(const RayTracingObject& ray_object)
{
	return m_bottom_level_acceleration_structures[ray_object.bottom_level_index].result_buffer;
}

const BufferResource& dx12rayobjectmanager::GetDataIndices() const
{
	return m_data_indices;
}

UINT dx12rayobjectmanager::BuildTopLevelAccelerationStructure(const std::vector<RayTracingObject>& objects)
{
	TopLevelAccelerationStructures top_level_acceleration_structure;

	top_level_acceleration_structure.instance_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * objects.size(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instancing_descs(objects.size());

	for (int i = 0; i < instancing_descs.size(); ++i)
	{
		BottomLevelAccelerationStructures* bottom_level_acceleration_structure = &m_bottom_level_acceleration_structures[objects[i].bottom_level_index];

		D3D12_RAYTRACING_INSTANCE_DESC instancing_desc = {};

		memcpy(instancing_desc.Transform, objects[i].instance_transform.m, sizeof(objects[i].instance_transform.m));
		instancing_desc.InstanceID = i;
		instancing_desc.InstanceMask = 0xFF;
		instancing_desc.InstanceContributionToHitGroupIndex = bottom_level_acceleration_structure->hit_shader_index;
		instancing_desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instancing_desc.AccelerationStructure = bottom_level_acceleration_structure->result_buffer.buffer->GetGPUVirtualAddress();

		instancing_descs[i] = instancing_desc;
	}

	D3D12_RANGE nothing = { 0, 0 };
	unsigned char* mappedPtr = nullptr;
	HRESULT hr = top_level_acceleration_structure.instance_buffer.buffer->Map(0, &nothing, reinterpret_cast<void**>(&mappedPtr));

	assert(SUCCEEDED(hr));

	memcpy(mappedPtr, instancing_descs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instancing_descs.size());
	top_level_acceleration_structure.instance_buffer.buffer->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.NumDescs = instancing_descs.size();
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.InstanceDescs = top_level_acceleration_structure.instance_buffer.buffer->GetGPUVirtualAddress();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	dx12core::GetDx12Core().GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &prebuildInfo);


	top_level_acceleration_structure.result_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, TextureType::TEXTURE_UAV);//dx12core::GetDx12Core().GetBufferManager()->CreateBuffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	top_level_acceleration_structure.scratch_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationStructureDesc;
	accelerationStructureDesc.DestAccelerationStructureData =
		top_level_acceleration_structure.result_buffer.buffer->GetGPUVirtualAddress();
	accelerationStructureDesc.Inputs = topLevelInputs;
	accelerationStructureDesc.SourceAccelerationStructureData = NULL;
	accelerationStructureDesc.ScratchAccelerationStructureData =
		top_level_acceleration_structure.scratch_buffer.buffer->GetGPUVirtualAddress();

	dx12core::GetDx12Core().GetDirectCommand()->BuildRaytracingAccelerationStructure(&accelerationStructureDesc);

	dx12core::GetDx12Core().GetDirectCommand()->ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_UAV, top_level_acceleration_structure.result_buffer.buffer.Get());

	m_top_level_acceleration_structures.push_back(top_level_acceleration_structure);

	return m_top_level_acceleration_structures.size() - 1;
}

void dx12rayobjectmanager::UpdateTopLevelAccelerationStructure(const std::vector<RayTracingObject>& objects, UINT scene_index)
{
	TopLevelAccelerationStructures top_level_acceleration_structure = m_top_level_acceleration_structures[0];

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instancing_descs(objects.size());

	for (int i = 0; i < instancing_descs.size(); ++i)
	{
		BottomLevelAccelerationStructures* bottom_level_acceleration_structure = &m_bottom_level_acceleration_structures[objects[i].bottom_level_index];

		D3D12_RAYTRACING_INSTANCE_DESC instancing_desc = {};

		memcpy(instancing_desc.Transform, objects[i].instance_transform.m, sizeof(objects[i].instance_transform.m));
		instancing_desc.InstanceID = i;
		instancing_desc.InstanceMask = 0xFF;
		instancing_desc.InstanceContributionToHitGroupIndex = bottom_level_acceleration_structure->hit_shader_index;
		instancing_desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instancing_desc.AccelerationStructure = bottom_level_acceleration_structure->result_buffer.buffer->GetGPUVirtualAddress();

		instancing_descs[i] = instancing_desc;
	}

	D3D12_RANGE nothing = { 0, 0 };
	unsigned char* mappedPtr = nullptr;
	HRESULT hr = top_level_acceleration_structure.instance_buffer.buffer->Map(0, &nothing, reinterpret_cast<void**>(&mappedPtr));

	assert(SUCCEEDED(hr));

	memcpy(mappedPtr, instancing_descs.data(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instancing_descs.size());
	top_level_acceleration_structure.instance_buffer.buffer->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.NumDescs = instancing_descs.size();
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.InstanceDescs = top_level_acceleration_structure.instance_buffer.buffer->GetGPUVirtualAddress();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationStructureDesc;
	accelerationStructureDesc.DestAccelerationStructureData =
		top_level_acceleration_structure.result_buffer.buffer->GetGPUVirtualAddress();
	accelerationStructureDesc.Inputs = topLevelInputs;
	accelerationStructureDesc.SourceAccelerationStructureData = NULL;
	accelerationStructureDesc.ScratchAccelerationStructureData =
		top_level_acceleration_structure.scratch_buffer.buffer->GetGPUVirtualAddress();

	dx12core::GetDx12Core().GetDirectCommand()->BuildRaytracingAccelerationStructure(&accelerationStructureDesc);

	dx12core::GetDx12Core().GetDirectCommand()->ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_UAV, top_level_acceleration_structure.result_buffer.buffer.Get());
}

UINT dx12rayobjectmanager::BuildBottomLevelAcceleratonStructure(DirectX::XMFLOAT3X4 instance_transform, UINT hit_shader_index)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometry_descriptions(m_meshes.size());

	for (int i = 0; i < m_meshes.size(); ++i)
	{
		assert(m_meshes[i].mesh.buffer->GetGPUVirtualAddress() % 16 == 0);

		geometry_descriptions[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometry_descriptions[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		geometry_descriptions[i].Triangles.Transform3x4 = m_meshes[i].transform.buffer.Get() == nullptr ? NULL : m_meshes[i].transform.buffer->GetGPUVirtualAddress();
		geometry_descriptions[i].Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
		geometry_descriptions[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometry_descriptions[i].Triangles.IndexCount = 0;
		geometry_descriptions[i].Triangles.VertexCount = m_meshes[i].mesh.nr_of_elements;
		geometry_descriptions[i].Triangles.IndexBuffer = NULL;
		geometry_descriptions[i].Triangles.VertexBuffer.StartAddress = m_meshes[i].mesh.buffer->GetGPUVirtualAddress();
		geometry_descriptions[i].Triangles.VertexBuffer.StrideInBytes = m_meshes[i].mesh.element_size;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	bottomLevelInputs.NumDescs = m_meshes.size();
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.pGeometryDescs = geometry_descriptions.data();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	dx12core::GetDx12Core().GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &prebuildInfo);

	BottomLevelAccelerationStructures bottom_level_acceleration_structure;
	bottom_level_acceleration_structure.result_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	bottom_level_acceleration_structure.scratch_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationStructureDesc;
	accelerationStructureDesc.DestAccelerationStructureData =
		bottom_level_acceleration_structure.result_buffer.buffer->GetGPUVirtualAddress();
	accelerationStructureDesc.Inputs = bottomLevelInputs;
	accelerationStructureDesc.SourceAccelerationStructureData = NULL;
	accelerationStructureDesc.ScratchAccelerationStructureData =
		bottom_level_acceleration_structure.scratch_buffer.buffer->GetGPUVirtualAddress();

	dx12core::GetDx12Core().GetDirectCommand()->BuildRaytracingAccelerationStructure(&accelerationStructureDesc);

	dx12core::GetDx12Core().GetDirectCommand()->ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_UAV, bottom_level_acceleration_structure.result_buffer.buffer.Get());

	bottom_level_acceleration_structure.hit_shader_index = hit_shader_index;

	m_bottom_level_acceleration_structures.push_back(bottom_level_acceleration_structure);

	return m_bottom_level_acceleration_structures.size() - 1;
}

UINT dx12rayobjectmanager::BuildBottomLevelAcceleratonStructureAABB(DirectX::XMFLOAT3X4 instance_transform, UINT hit_shader_index)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometry_descriptions(m_aabbs.size());

	for (int i = 0; i < geometry_descriptions.size(); ++i)
	{
		geometry_descriptions[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
		geometry_descriptions[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		geometry_descriptions[i].AABBs.AABBCount = 1;
		geometry_descriptions[i].AABBs.AABBs.StartAddress = m_aabbs[i].aabb.buffer->GetGPUVirtualAddress();
		geometry_descriptions[i].AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	bottomLevelInputs.NumDescs = geometry_descriptions.size();
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	bottomLevelInputs.pGeometryDescs = geometry_descriptions.data();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	dx12core::GetDx12Core().GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &prebuildInfo);

	BottomLevelAccelerationStructures bottom_level_acceleration_structure;
	bottom_level_acceleration_structure.result_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
	bottom_level_acceleration_structure.scratch_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationStructureDesc;
	accelerationStructureDesc.DestAccelerationStructureData =
		bottom_level_acceleration_structure.result_buffer.buffer->GetGPUVirtualAddress();
	accelerationStructureDesc.Inputs = bottomLevelInputs;
	accelerationStructureDesc.SourceAccelerationStructureData = NULL;
	accelerationStructureDesc.ScratchAccelerationStructureData =
		bottom_level_acceleration_structure.scratch_buffer.buffer->GetGPUVirtualAddress();

	dx12core::GetDx12Core().GetDirectCommand()->BuildRaytracingAccelerationStructure(&accelerationStructureDesc);

	dx12core::GetDx12Core().GetDirectCommand()->ResourceBarrier(D3D12_RESOURCE_BARRIER_TYPE_UAV, bottom_level_acceleration_structure.result_buffer.buffer.Get());

	bottom_level_acceleration_structure.hit_shader_index = hit_shader_index;

	m_bottom_level_acceleration_structures.push_back(bottom_level_acceleration_structure);

	return m_bottom_level_acceleration_structures.size() - 1;
}
