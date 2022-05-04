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
		UINT bottom_level_index;
		BufferResource result_buffer;
		BufferResource scratch_buffer;
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
	UINT BuildTopLevelAccelerationStructure(UINT bottom_level_index);

	UINT BuildBottomLevelAcceleratonStructure();
public:
	dx12rayobjectmanager();
	~dx12rayobjectmanager();
	void AddMesh(BufferResource mesh_buffer, BufferResource transform);
	RayTracingObject CreateRayTracingObject();
	const BufferResource& GetTopLevelResultAccelerationStructureBuffer(const RayTracingObject& ray_object);
	const BufferResource& GetTopLevelScratchAccelerationStructureBuffer(const RayTracingObject& ray_object);
	const BufferResource& GetTopLevelInstanceBuffer(const RayTracingObject& ray_object);
	const BufferResource& GetBottomLevelScratchAccelerationStructureBuffer(const RayTracingObject& ray_object);
};

