#pragma once
#include "dx12buffermanager.h"
#include <vector>
#include <map>

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
private:
	std::vector<BottomLevelAccelerationStructures> m_bottom_level_acceleration_structures;
	std::vector<TopLevelAccelerationStructures> m_top_level_acceleration_structures;
	std::map<UINT64, RayTracingObject> m_existing_objects;

private:
	UINT BuildBottomLevelAccelerationStructure(BufferResource* vertex_buffer);
	UINT BuildTopLevelAccelerationStructure(UINT bottom_level_index);
public:
	dx12rayobjectmanager();
	~dx12rayobjectmanager();
	RayTracingObject CreateRayTracingObject(BufferResource* vertex_buffer);
	ID3D12Resource* GetTopLevelResultAccelerationStructureBuffer();
	ID3D12Resource* GetTopLevelScratchAccelerationStructureBuffer();
	ID3D12Resource* GetTopLevelInstanceBuffer();
	ID3D12Resource* GetBottomLevelScratchAccelerationStructureBuffer();
};

