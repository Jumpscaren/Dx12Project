RaytracingAccelerationStructure scene : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

static const float3 HIT_COLOUR = float3(0.0f, 0.0f, 1.0f);
static const float3 MISS_COLOUR = float3(1.0f, 0.0f, 0.0f);
static const float3 CLEAR_COLOUR = float3(0.0f, 0.0f, 0.0f);

struct RayPayloadData
{
	float3 colour;
};

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

	RayPayloadData payload = { float3(0.0f, 0.0f, 0.0f) };

	TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

	if(all(outputTexture[currentPixel].xyz == CLEAR_COLOUR) || all(payload.colour == HIT_COLOUR))
		outputTexture[currentPixel] = float4(payload.colour, 1.0f);
}

[shader("miss")]
void MissShader(inout RayPayloadData data)
{
	data.colour = float3(1.0f, 0.0f, 0.0f);
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayloadData data, in BuiltInTriangleIntersectionAttributes attribs)
{
	data.colour = float3(0.0f, 0.0f, 1.0f);
}