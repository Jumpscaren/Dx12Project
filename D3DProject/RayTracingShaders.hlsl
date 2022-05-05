#include "RayTracingShaderHeader.h"

RaytracingAccelerationStructure scene : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

static const float3 HIT_COLOUR = float3(0.0f, 0.0f, 1.0f);
static const float3 MISS_COLOUR = float3(1.0f, 0.0f, 0.0f);
static const float3 CLEAR_COLOUR = float3(0.0f, 0.0f, 0.0f);

//struct RayPayloadData
//{
//	float3 colour;
//};

[shader("raygeneration")]
void RayGenerationShader()
{

	uint3 dispatchDim = DispatchRaysDimensions();
	uint2 currentPixel = DispatchRaysIndex().xy;
	float3 origin = float3(-1 + (2.0f * currentPixel.x) / dispatchDim.x,
		1 - (2.0f * currentPixel.y) / dispatchDim.y, -1.0f);
	float2 d = (((currentPixel.xy + 0.5f) / dispatchDim.xy) * 2.f - 1.f);

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = float3(0.0f, 0.0f, 1.0f);
	ray.TMin = 0.0f;
	ray.TMax = 1000.0f;

	RayPayloadData payload = { float3(0.0f, 0.0f, 0.0f), 0 };

	TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

	//TraceRay(scene, );

	//if(all(outputTexture[currentPixel].xyz == CLEAR_COLOUR) || all(payload.colour == HIT_COLOUR))
	//	outputTexture[currentPixel] = float4(payload.colour, 1.0f);

	if (payload.max_count != -1)
		outputTexture[currentPixel] = float4(payload.colour, 1.0f);

	//outputTexture[currentPixel] = float4(payload, 1.0f);
}

[shader("miss")]
void MissShader(inout RayPayloadData data)
{
	data.colour = float3(1.0f, 0.0f, 0.0f);
	data.max_count = -1;
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayloadData data, in BuiltInTriangleIntersectionAttributes attribs)
{
	//if (data.max_count > 2)
		//return;

	float3 t = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

	RayDesc ray;
	ray.Origin = t;
	ray.Direction = float3(0.0f, 0.0f, 1.0f);
	ray.TMin = 0.0f;
	ray.TMax = 1000.0f;

	//RayPayloadData payload = { float3(0.0f, 0.0f, 0.0f) };
	data.max_count += 1;

	int index = InstanceID();//InstanceIndex();//PrimitiveIndex();

	if (index == 0)
		data.colour = float3(1.0f, 0.0f, 0.0f);
	else
		data.colour = float3(0.0f, 0.0f, 1.0f);

	//TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, data);

	//data.colour = float3(index, index, index);//float3(0.0f, 0.0f, 1.0f);
}