#pragma once
#include "dx12buffermanager.h"
#include <vector>
#include <map>
#include <DirectXMath.h>

struct RayTracingObject
{
	UINT bottom_level_index;
	UINT instance_index;
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
private:
	std::vector<BottomLevelAccelerationStructures> m_bottom_level_acceleration_structures;
	std::vector<TopLevelAccelerationStructures> m_top_level_acceleration_structures;

	std::vector<AddedMesh> m_meshes;

private:
	UINT BuildTopLevelAccelerationStructure(const std::vector<RayTracingObject>& objects);
	void UpdateTopLevelAccelerationStructure(const std::vector<RayTracingObject>& objects, UINT scene_index);

	UINT BuildBottomLevelAcceleratonStructure();
public:
	dx12rayobjectmanager();
	~dx12rayobjectmanager();
	void AddMesh(BufferResource mesh_buffer, BufferResource transform = {});
	RayTracingObject CreateRayTracingObject();
	void CreateScene(const std::vector<RayTracingObject>& objects);
	void UpdateScene(const std::vector<RayTracingObject>& objects);
	const BufferResource& GetTopLevelResultAccelerationStructureBuffer();
	const BufferResource& GetTopLevelScratchAccelerationStructureBuffer();
	const BufferResource& GetTopLevelInstanceBuffer();
	const BufferResource& GetBottomLevelScratchAccelerationStructureBuffer(const RayTracingObject& ray_object);
};

