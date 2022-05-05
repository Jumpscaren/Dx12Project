#pragma once
#include "dx12buffermanager.h"
#include <vector>
#include <map>
#include <DirectXMath.h>

struct RayTracingObject
{
	UINT object;
};

class dx12rayobjectmanager
{
private:
	struct BottomLevelAccelerationStructures
	{
		BufferResource result_buffer;
		BufferResource scratch_buffer;
	};
	struct TopLevelAccelerationStructures
	{
		//UINT bottom_level_index;
		std::vector<UINT> bottom_level_indices;
		BufferResource result_buffer;
		BufferResource scratch_buffer;
		std::vector<BufferResource> instance_buffers;
		BufferResource instance_buffer;
	};
	struct AddedMesh
	{
		BufferResource mesh;
		BufferResource transform;
	};
private:
	std::vector<BottomLevelAccelerationStructures> m_bottom_level_acceleration_structures;
	std::vector<TopLevelAccelerationStructures> m_top_level_acceleration_structures;
	std::map<UINT64, RayTracingObject> m_existing_objects;

	std::vector<AddedMesh> m_meshes;

private:
	UINT BuildBottomLevelAccelerationStructure(BufferResource* vertex_buffer);
	UINT BuildTopLevelAccelerationStructure(const std::vector<UINT>& bottom_level_indices);

	UINT BuildBottomLevelAcceleratonStructure(UINT meshes_count, UINT mesh_index);
public:
	dx12rayobjectmanager();
	~dx12rayobjectmanager();
	void AddMesh(BufferResource mesh_buffer, BufferResource transform);
	RayTracingObject CreateRayTracingObject(bool one_bottom_acceleration_structure);
	const BufferResource& GetTopLevelResultAccelerationStructureBuffer(const RayTracingObject& ray_object);
	const BufferResource& GetTopLevelScratchAccelerationStructureBuffer(const RayTracingObject& ray_object);
	const BufferResource& GetTopLevelInstanceBuffer(const RayTracingObject& ray_object);
	const BufferResource& GetBottomLevelScratchAccelerationStructureBuffer(const RayTracingObject& ray_object);
};

