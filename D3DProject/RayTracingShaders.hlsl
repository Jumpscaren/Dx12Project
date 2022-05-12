#include "RayTracingShaderHeader.h"

RaytracingAccelerationStructure scene : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

static const float3 HIT_COLOUR = float3(0.0f, 0.0f, 1.0f);
static const float3 MISS_COLOUR = float3(0.0f, 0.0f, 0.0f);
static const float3 CLEAR_COLOUR = float3(0.0f, 0.0f, 0.0f);

//struct RayPayloadData
//{
//	float3 colour;
//};

struct TriangleColour
{
	float3 colour;
};

StructuredBuffer<TriangleColour> colours : register(t1);
StructuredBuffer<SphereAABB> sphere_AABBs : register(t2);

cbuffer ViewProjectionMatrix : register(b0)
{
	float4x4 view_projection_matrix;
	float3 camera_position;
}

[shader("raygeneration")]
void RayGenerationShader()
{

	uint3 dispatchDim = DispatchRaysDimensions();
	uint2 currentPixel = DispatchRaysIndex().xy;
	//float3 origin = float3(-1 + (2.0f * currentPixel.x) / dispatchDim.x,
	//	1 - (2.0f * currentPixel.y) / dispatchDim.y, -1.0f);
	//float2 d = (((currentPixel.xy + 0.5f) / dispatchDim.xy) * 2.f - 1.f);

	//float3 camera_position = float3(0,0,-10.0f);

	//float direction = normalize(origin - camera_position);

	//float3 origin = float3(currentPixel.x, 1 - currentPixel.y, 0.0f);

	float2 d = currentPixel.xy + 0.5;

	// Screen position for the ray
	float2 screenPos = d / dispatchDim.xy * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates
	screenPos.y = -screenPos.y;

	float3 origin = float3(screenPos, 3) + camera_position;

	float4 world = mul(float4(origin, 1), view_projection_matrix);
	world.xyz /= world.w;

	origin = camera_position;

	float3 direction = normalize(world.xyz - origin);

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;//float3(0.0f, 0.0f, 1.0f);
	ray.TMin = 0.0f;
	ray.TMax = 1000.0f;

	RayPayloadData payload = { float3(0.0f, 0.0f, 0.0f), 0};

	TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

	//if(all(outputTexture[currentPixel].xyz == CLEAR_COLOUR) || all(payload.colour == HIT_COLOUR))
	//	outputTexture[currentPixel] = float4(payload.colour, 1.0f);

	if (payload.max_count != -1)
		outputTexture[currentPixel] = float4(payload.colour, 1.0f);

	//outputTexture[currentPixel] = float4(payload, 1.0f);
}

[shader("miss")]
void MissShader(inout RayPayloadData data)
{
	data.colour = float3(0.0f, 0.0f, 0.0f);
	data.max_count = -1;
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayloadData data, in BuiltInTriangleIntersectionAttributes attribs)
{
	//if (data.max_count > 2)
		//return;

	//float3 t = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

	//RayDesc ray;
	//ray.Origin = t;
	//ray.Direction = float3(0.0f, 0.0f, -1.0f);
	//ray.TMin = 0.0f;
	//ray.TMax = 1000.0f;

	//TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, data);

	//RayPayloadData payload = { float3(0.0f, 0.0f, 0.0f) };
	//data.max_count += 1;

	int index = InstanceID();
	//int index = InstanceIndex();
	//int index = PrimitiveIndex();

	data.colour = colours[index].colour;

	data.colour *= float3(attribs.barycentrics.xy, 1.0f);

	//if (index == 0)
	//	data.colour = float3(1.0f, 0.0f, 0.0f);
	//else if (index == 1)
	//	data.colour = float3(0.0f, 0.0f, 1.0f);
	//else 
	//	data.colour = float3(0.0f, 1.0f, 0.0f);

	//data.colour = triangle_colour;

	//TraceRay(scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, data);

	//data.colour = float3(index, index, index);//float3(0.0f, 0.0f, 1.0f);
}

struct SphereNormal
{
	float3 sphere_hit_point;
	float3 sphere_normal;
};

[shader("closesthit")]
void ReflectionClosestHitShader(inout RayPayloadData data, in SphereNormal attribs)
{
	//data.colour = attribs.sphere_normal;
	//return;

	data.max_count += 1;

	if (data.max_count < 2)
	{
		float3 t = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

		RayDesc ray;
		ray.Origin = attribs.sphere_hit_point;
		ray.Direction = reflect(WorldRayDirection(), attribs.sphere_normal);//float3(0.0f, 0.0f, -1.0f);//
		ray.TMin = 0.0f;
		ray.TMax = 1000.0f;

		TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, data);

		//data.colour = float3(RayTCurrent(), 0.0f, 0.0f) / 10.0f;
	}

	if (data.max_count == -1)
	{
		data.colour = attribs.sphere_normal;
		data.max_count = 0;
	}

	//if (all(data.colour.xyz == float3(0.0f, 0.0f, 0.0f)))
	//	data.colour = attribs.sphere_normal;
}

[shader("intersection")]
void ReflectionIntersectionShader()
{
	//return;

	float tHit = 0;

	float3 position = sphere_AABBs[InstanceID()-1].position;
	float radius = sphere_AABBs[InstanceID() - 1].radius;

	float3 f = ObjectRayOrigin() - position;
	float b = dot(ObjectRayDirection(), f);
	float c = dot(f, f) - radius * radius;

	if (b * b - c < 0)
		return;

	float sqrt_val = sqrt(b*b - c);
	float t1 = -b - sqrt_val;
	float t2 = -b + sqrt_val;

	if (t1 < 0 && t2 < 0)
		return;

	if (t1 < 0)
		tHit = t2;
	else
		tHit = t1;

	SphereNormal attr;
	attr.sphere_normal = ObjectRayOrigin() + ObjectRayDirection() * tHit;
	attr.sphere_normal = normalize(attr.sphere_normal - position);					//float3(0, 0, 0);
	//attr.sphere_normal = reflect(ObjectRayDirection(), attr.sphere_normal);
	attr.sphere_hit_point = ObjectRayOrigin() + ObjectRayDirection() * tHit;
	ReportHit(tHit, 0, attr);
}