#pragma once
#include "dx12buffermanager.h"
#include <vector>
#include <map>
#include <DirectXMath.h>

struct RayTracingObject
{
	UINT bottom_level_index;
	UINT data_index;
	DirectX::XMFLOAT3X4 instance_transform;
};

class dx12rayobjectmanager
{
private:
	struct BottomLevelAccelerationStructures
	{
		BufferResource result_buffer;
		BufferResource scratch_buffer;
		UINT hit_shader_index;
	};
	struct TopLevelAccelerationStructures
	{
		std::vector<UINT> bottom_level_indices;
		BufferResource result_buffer;
		BufferResource scratch_buffer;
		BufferResource instance_buffer;
	};
	struct AddedMesh
	{
		BufferResource mesh;
		BufferResource transform;
	};
	struct AddedProceduralGeometry
	{
		BufferResource aabb;
	};
private:
	std::vector<BottomLevelAccelerationStructures> m_bottom_level_acceleration_structures;
	std::vector<TopLevelAccelerationStructures> m_top_level_acceleration_structures;

	std::vector<AddedMesh> m_meshes;
	std::vector<AddedProceduralGeometry> m_aabbs;

	BufferResource m_data_indices;
	//std::vector<UINT> m;

private:
	UINT BuildTopLevelAccelerationStructure(const std::vector<RayTracingObject>& objects);
	void UpdateTopLevelAccelerationStructure(const std::vector<RayTracingObject>& objects, UINT scene_index);

	UINT BuildBottomLevelAcceleratonStructure(DirectX::XMFLOAT3X4 instance_transform, UINT hit_shader_index);
	UINT BuildBottomLevelAcceleratonStructureAABB(DirectX::XMFLOAT3X4 instance_transform, UINT hit_shader_index);
public:
	dx12rayobjectmanager();
	~dx12rayobjectmanager();
	void AddMesh(BufferResource mesh_buffer, BufferResource transform = {});
	void AddAABB(BufferResource aabb_buffer);
	RayTracingObject CreateRayTracingObject(UINT hit_shader_index = 0, DirectX::XMFLOAT3X4 instance_transform = {}, UINT data_index = 0);
	RayTracingObject CreateRayTracingObjectAABB(UINT hit_shader_index = 0, DirectX::XMFLOAT3X4 instance_transform = {}, UINT data_index = 0);
	RayTracingObject CopyRayTracingObjectAABB(RayTracingObject& ray_tracing_object, DirectX::XMFLOAT3X4 instance_transform = {}, UINT data_index = 0);
	void CreateScene(const std::vector<RayTracingObject>& objects);
	void UpdateScene(const std::vector<RayTracingObject>& objects);
	const BufferResource& GetTopLevelResultAccelerationStructureBuffer();
	const BufferResource& GetTopLevelScratchAccelerationStructureBuffer();
	const BufferResource& GetTopLevelInstanceBuffer();
	const BufferResource& GetBottomLevelScratchAccelerationStructureBuffer(const RayTracingObject& ray_object);
	const BufferResource& GetDataIndices() const;
};

