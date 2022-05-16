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
StructuredBuffer<uint> data_indices : register(t3);

cbuffer ViewProjectionMatrix : register(b0)
{
	float4x4 view_projection_matrix;
	float4x4 view_inverse_matrix;
	float4x4 projection_inverse_matrix;
	float4x4 view_matrix;
	float4x4 projection_matrix;
	float3 camera_position;
	float max_recursion;
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

	//float2 d = currentPixel.xy + 0.5;

	// Screen position for the ray
	//float2 screenPos = d / float2(dispatchDim.xy) * 2.0 - 1.0;

	// Invert Y for DirectX-style coordinates
	//screenPos.y = -screenPos.y;

	//float3 origin = float3(screenPos, 0.0f); //+ camera_position;

	//float4 world = mul(float4(screenPos.xy, 0.0f, 1), view_projection_matrix);
	//world.xyz /= world.w;

	//origin = camera_position;

	//float3 direction = normalize(world.xyz - origin);


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
	ray.Direction = direction;//float3(0.0f, 0.0f, 1.0f);
	ray.TMin = 0.01f;
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

	int index = data_indices[InstanceID()];
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

	if (data.max_count < max_recursion)
	{
		float3 t = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();

		RayDesc ray;
		ray.Origin = attribs.sphere_hit_point;
		ray.Direction = attribs.sphere_normal;//reflect(WorldRayDirection(), attribs.sphere_normal);//float3(0.0f, 0.0f, -1.0f);//
		ray.TMin = 0.01f;
		ray.TMax = 1000.0f;

		TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, data);

		//data.colour = float3(RayTCurrent(), 0.0f, 0.0f) / 10.0f;
	}
	else
	{
		uint data_index = data_indices[InstanceID() % 3];
		data.colour = colours[data_index].colour * attribs.sphere_normal;
	}

	if (data.max_count == -1)
	{
		//data.colour = attribs.sphere_normal;
		uint data_index = data_indices[InstanceID() % 3];
		data.colour = colours[data_index].colour * attribs.sphere_normal;
		data.max_count = 0;
	}

	//if (all(data.colour.xyz == float3(0.0f, 0.0f, 0.0f)))
	//	data.colour = attribs.sphere_normal;
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

	//float tHit = 0;

	//float3 position = sphere_AABBs[InstanceID() - 1].position;

	////float4 g = mul(view_matrix, float4(WorldRayOrigin().xyz, 1));
	////float3 ray_origin = g.xyz;
	////g = mul(view_matrix, float4(WorldRayDirection().xyz, 0));
	////g = mul(projection_matrix, float4(g.xyz, 0));
	////float3 ray_direction = g.xyz;

	//float3 ray_origin = WorldRayOrigin();
	//float3 ray_direction = WorldRayDirection();

	////ray_origin = mul(float4(ObjectRayOrigin().xyz, 1.0f), view_matrix).xyz;
	////float3 g = mul(float4(ObjectRayDirection().xyz, 0), projection_matrix).xyz;
	////g = mul(float4(g.xyz, 0), view_matrix).xyz;
	////ray_direction = g.xyz;

	////ray_origin = mul(float4(ObjectRayOrigin().xyz, 1.0f), view_projection_matrix).xyz;
	////float3 g = mul(float4(ObjectRayDirection().xyz, 0), view_projection_matrix).xyz;
	////ray_direction = g.xyz;


	//float radius = sphere_AABBs[InstanceID() - 1].radius;

	//float3 f = ray_origin - position;
	//float b = dot(ray_direction, f);
	//float c = dot(f, f) - radius * radius;

	//if (b * b - c < 0)
	//	return;

	//float sqrt_val = sqrt(b * b - c);
	//float t1 = -b - sqrt_val;
	//float t2 = -b + sqrt_val;

	//if (t1 < 0 && t2 < 0)
	//	return;

	//if (t1 < 0)
	//	tHit = t2;
	//else
	//	tHit = t1;

	//SphereNormal attr;
	//attr.sphere_normal = ray_origin + ray_direction * tHit;
	//attr.sphere_normal = normalize(attr.sphere_normal - position);
	////attr.sphere_hit_point = ray_origin + ray_direction * tHit;
	//ReportHit(tHit, 0, attr);
}