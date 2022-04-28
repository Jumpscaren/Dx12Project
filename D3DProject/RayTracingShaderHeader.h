#pragma once

#ifdef __cplusplus
#include <DirectXMath.h>
typedef DirectX::XMFLOAT3 float3;
#else
#endif

struct RayPayloadData
{
	float3 colour;
};