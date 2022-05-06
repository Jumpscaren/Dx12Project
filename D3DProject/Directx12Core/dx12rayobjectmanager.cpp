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
}

void dx12rayobjectmanager::AddMesh(BufferResource mesh_buffer, BufferResource transform)
{
	//BufferResource transform_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*) (&transform), sizeof(DirectX::XMFLOAT3X4), 1);

	AddedMesh mesh = { mesh_buffer, transform };
	m_meshes.push_back(mesh);
}

RayTracingObject dx12rayobjectmanager::CreateRayTracingObject()
{
	//std::vector<UINT> bottom_level_indices;
	////UINT bottom_level_index = 0;
	//if (one_bottom_acceleration_structure)
	//{
	//	UINT bottom_level_index = BuildBottomLevelAcceleratonStructure(m_meshes.size(), -1);
	//	bottom_level_indices.push_back(bottom_level_index);
	//}
	//else
	//{
	//	for (int i = 0; i < m_meshes.size(); ++i)
	//	{
	//		UINT bottom_level_index = BuildBottomLevelAcceleratonStructure(1, i);
	//		bottom_level_indices.push_back(bottom_level_index);
	//	}
	//}
	//UINT bottom_level_index = BuildBottomLevelAcceleratonStructure();

	UINT bottom_level_index = BuildBottomLevelAcceleratonStructure();

	//UINT top_level_index = BuildTopLevelAccelerationStructure(bottom_level_indices);

    //UINT bottom_level_index = BuildBottomLevelAccelerationStructure(vertex_buffer);
    //UINT top_level_index = BuildTopLevelAccelerationStructure(bottom_level_index);

	RayTracingObject object = { bottom_level_index, bottom_level_index };

	////If it does not already exist then we insert it
	//auto existing_object_it = m_existing_objects.find((UINT64)vertex_buffer->buffer.Get());
	//if (existing_object_it == m_existing_objects.end())
	//	m_existing_objects.insert({ (UINT64)vertex_buffer->buffer.Get(), object });

	m_meshes.clear();

	return object;
}

void dx12rayobjectmanager::CreateScene(const std::vector<RayTracingObject>& objects)
{
	BuildTopLevelAccelerationStructure(objects);
}

void dx12rayobjectmanager::UpdateScene(const std::vector<RayTracingObject>& objects)
{
	UpdateTopLevelAccelerationStructure(objects, 0);
}

const BufferResource& dx12rayobjectmanager::GetTopLevelResultAccelerationStructureBuffer()
{
	//return m_top_level_acceleration_structures[ray_object.object].result_buffer;
	return m_top_level_acceleration_structures[0].result_buffer;
}

const BufferResource& dx12rayobjectmanager::GetTopLevelScratchAccelerationStructureBuffer()
{
	//return m_top_level_acceleration_structures[ray_object.object].scratch_buffer;
	return m_top_level_acceleration_structures[0].scratch_buffer;
}

const BufferResource& dx12rayobjectmanager::GetTopLevelInstanceBuffer()
{
	//return m_top_level_acceleration_structures[ray_object.object].instance_buffer;
	return m_top_level_acceleration_structures[0].instance_buffer;
}

const BufferResource& dx12rayobjectmanager::GetBottomLevelScratchAccelerationStructureBuffer(const RayTracingObject& ray_object)
{
	//UINT index = m_top_level_acceleration_structures[ray_object.object].bottom_level_indices[0];
	return m_bottom_level_acceleration_structures[ray_object.bottom_level_index].result_buffer;
}

UINT dx12rayobjectmanager::BuildTopLevelAccelerationStructure(const std::vector<RayTracingObject>& objects)//UINT bottom_level_index)
{
	TopLevelAccelerationStructures top_level_acceleration_structure;

	//top_level_acceleration_structure.bottom_level_indices = bottom_level_indices;

	top_level_acceleration_structure.instance_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * objects.size(), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD);

	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instancing_descs(objects.size());

	for (int i = 0; i < instancing_descs.size(); ++i)
	{
		BottomLevelAccelerationStructures* bottom_level_acceleration_structure = &m_bottom_level_acceleration_structures[objects[i].bottom_level_index];

		D3D12_RAYTRACING_INSTANCE_DESC instancingDesc = {};

		instancingDesc.Transform[0][0] = instancingDesc.Transform[1][1] =
			instancingDesc.Transform[2][2] = 1;
		instancingDesc.InstanceID = i;
		instancingDesc.InstanceMask = 0xFF;
		instancingDesc.InstanceContributionToHitGroupIndex = 0;
		instancingDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instancingDesc.AccelerationStructure = bottom_level_acceleration_structure->result_buffer.buffer->GetGPUVirtualAddress();

		instancing_descs[i] = instancingDesc;
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

		D3D12_RAYTRACING_INSTANCE_DESC instancingDesc = {};

		instancingDesc.Transform[0][0] = instancingDesc.Transform[1][1] =
			instancingDesc.Transform[2][2] = 1;
		instancingDesc.InstanceID = i;
		instancingDesc.InstanceMask = 0xFF;
		instancingDesc.InstanceContributionToHitGroupIndex = 0;
		instancingDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		instancingDesc.AccelerationStructure = bottom_level_acceleration_structure->result_buffer.buffer->GetGPUVirtualAddress();

		instancing_descs[i] = instancingDesc;
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

UINT dx12rayobjectmanager::BuildBottomLevelAcceleratonStructure()
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometry_descriptions(m_meshes.size()); //D3D12_RAYTRACING_GEOMETRY_DESC geometryDescriptions[1];

	for (int i = 0; i < m_meshes.size(); ++i)
	{
		assert(m_meshes[i].mesh.buffer->GetGPUVirtualAddress() % 16 == 0);

		geometry_descriptions[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometry_descriptions[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		geometry_descriptions[i].Triangles.Transform3x4 = m_meshes[i].transform.buffer.Get() == nullptr ? NULL : m_meshes[i].transform.buffer->GetGPUVirtualAddress();//NULL;
		geometry_descriptions[i].Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
		geometry_descriptions[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometry_descriptions[i].Triangles.IndexCount = 0;
		geometry_descriptions[i].Triangles.VertexCount = m_meshes[i].mesh.nr_of_elements; //vertex_buffer->nr_of_elements;
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

	m_bottom_level_acceleration_structures.push_back(bottom_level_acceleration_structure);

	return m_bottom_level_acceleration_structures.size() - 1;
}