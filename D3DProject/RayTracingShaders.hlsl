#include "RayTracingShaderHeader.h"

RaytracingAccelerationStructure scene : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

StructuredBuffer<TriangleColour> colours : register(t1);
StructuredBuffer<SphereAABB> sphere_AABBs : register(t2);
StructuredBuffer<uint> data_indices : register(t3);

cbuffer ViewProjectionMatrix : register(b0)
{
	float4x4 view_inverse_matrix;
	float4x4 projection_inverse_matrix;
	float3 camera_position;
	float max_recursion;
}

[shader("raygeneration")]
void RayGenerationShader()
{

	uint3 dispatchDim = DispatchRaysDimensions();
	uint2 currentPixel = DispatchRaysIndex().xy;;

	float2 d = currentPixel.xy + 0.5;

	// Screen position for the ray
	float2 screenPos = d / float2(dispatchDim.xy) * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates
	screenPos.y = -screenPos.y;
	float3 origin = mul(view_inverse_matrix, float4(0, 0, 0, 1));
	float4 target = mul(projection_inverse_matrix, float4(screenPos.x, screenPos.y, 1, 1));
	float3 direction = normalize(mul(view_inverse_matrix, float4(target.xyz, 0)).xyz);


	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.01f;
	ray.TMax = 1000.0f;

	RayPayloadData payload = { float3(0.0f, 0.0f, 0.0f), 0};

	TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

	if (payload.max_count != -1)
		outputTexture[currentPixel] = float4(payload.colour, 1.0f);
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
	int index = data_indices[InstanceID()];

	data.colour = colours[index].colour;

	data.colour *= float3(attribs.barycentrics.xy, 1.0f);
}

[shader("closesthit")]
void ReflectionClosestHitShader(inout RayPayloadData data, in SphereNormal attribs)
{
	data.max_count += 1;

	if (data.max_count < max_recursion)
	{
		float3 t = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

		RayDesc ray;
		ray.Origin = attribs.sphere_hit_point;
		ray.Direction = attribs.sphere_normal;
		ray.TMin = 0.01f;
		ray.TMax = 1000.0f;

		TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, data);
	}
	else
	{
		uint data_index = data_indices[InstanceID() % 3];
		data.colour = colours[data_index].colour * attribs.sphere_normal;
	}

	if (data.max_count == -1)
	{
		uint data_index = data_indices[InstanceID() % 3];
		data.colour = colours[data_index].colour * attribs.sphere_normal;
		data.max_count = 0;
	}
}

[shader("intersection")]
void ReflectionIntersectionShader()
{
	float3 ray_origin = WorldRayOrigin();
	float3 ray_direction = WorldRayDirection();

	float tHit = 0;

	uint data_index = data_indices[InstanceID()];

	float3 position = sphere_AABBs[data_index].position;
	float radius = sphere_AABBs[data_index].radius;

	float3 f = ray_origin - position;
	float b = dot(ray_direction, f);
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
	attr.sphere_normal = ray_origin + ray_direction * tHit;
	attr.sphere_normal = normalize(attr.sphere_normal - position);			
	attr.sphere_normal = reflect(ray_direction, attr.sphere_normal);;
	attr.sphere_hit_point = ray_origin + ray_direction * tHit;
	ReportHit(tHit, 0, attr);
}