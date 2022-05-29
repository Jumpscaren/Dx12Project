#pragma once

#ifdef __cplusplus
#include <DirectXMath.h>
typedef DirectX::XMFLOAT3 float3;
#else
#endif

struct RayPayloadData
{
	float3 colour;
	int max_count;
};

struct SphereAABB
{
	float3 position;
	float radius;
};

struct TriangleColour
{
	float3 colour;
};

struct SphereNormal
{
	float3 sphere_hit_point;
	float3 sphere_normal;
};