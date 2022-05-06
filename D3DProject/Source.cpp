#include <iostream>
#include "Window.h"
#include "Directx12Core/dx12core.h"
#include "RayTracingShaderHeader.h"

int main()
{
	std::cout << "Hello World\n";

	Window window(1280, 720, L"Project", L"Project");
	
	dx12core::GetDx12Core().Init(window.GetWindowHandle(), 2);

	TCHAR pwd[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, pwd);
	std::wstring gf(pwd);
	std::string of(gf.begin(), gf.end());
	std::cout << of.c_str() << "\n";

	TextureResource texture = dx12core::GetDx12Core().GetTextureManager()->CreateTexture2D("Textures/image.png", TextureType::TEXTURE_SRV);

	struct SimpleVertex
	{
		float position[3];
		float uv[2];
	};
	SimpleVertex triangle[3] =
	{
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},
		{{0.0f, 0.5f, 0.0f}, {0.5f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}}
	};
	SimpleVertex triangle_2[3] =
	{
		{{-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}
	};
	SimpleVertex quad[6] =
	{
		{{-0.75f, -0.75f, 0.5f}, {0.0f, 1.0f}},
		{{-0.75f, 0.75f, 0.5f}, {0.0f, 0.0f}},
		{{0.75f, 0.75f, 0.5f}, {1.0f, 0.0f}},

		{{-0.75f, -0.75f, 0.5f}, {0.0f, 1.0f}},
		{{0.75f, 0.75f, 0.5f}, {1.0f, 0.0f}},
		{{0.75f, -0.75f, 0.5f}, {1.0f, 1.0f}},
	};

	BufferResource vertex = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(triangle, sizeof(SimpleVertex), 3, TextureType::TEXTURE_SRV);
	BufferResource vertex_2 = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(triangle_2, sizeof(SimpleVertex), 3, TextureType::TEXTURE_SRV);
	BufferResource quad_mesh = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(quad, sizeof(SimpleVertex), 6, TextureType::TEXTURE_SRV);

	DirectX::XMFLOAT3X4 transform;
	DirectX::XMStoreFloat3x4(&transform, DirectX::XMMatrixTranslation(0,0,0));
	DirectX::XMFLOAT3X4 transform_2;
	DirectX::XMMATRIX transform_matrix_2 = DirectX::XMMatrixTranslation(0, 0.5f, 0) * DirectX::XMMatrixRotationZ(10);
	DirectX::XMStoreFloat3x4(&transform_2, transform_matrix_2);

	BufferResource vertex_transform = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&transform), sizeof(DirectX::XMFLOAT3X4), 1);
	BufferResource vertex_2_transform = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&transform_2), sizeof(DirectX::XMFLOAT3X4), 1);

	dx12core::GetDx12Core().GetDirectCommand()->Execute();
	dx12core::GetDx12Core().GetDirectCommand()->SignalAndWait();

	dx12renderpipeline* render_pipeline = new dx12renderpipeline();
	render_pipeline->AddStructuredBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX, false);
	render_pipeline->AddShaderResource(0, D3D12_SHADER_VISIBILITY_PIXEL, false);
	render_pipeline->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, D3D12_SHADER_VISIBILITY_PIXEL, false);

	render_pipeline->CreateRenderPipeline("x64/Debug/VertexShader1.cso", "x64/Debug/PixelShader1.cso");

	auto object = render_pipeline->CreateRenderObject();
	object->AddStructuredBuffer(quad_mesh, 0, D3D12_SHADER_VISIBILITY_VERTEX, false);
	object->AddShaderResourceView(texture, 0, D3D12_SHADER_VISIBILITY_PIXEL, false);
	object->SetMeshData(quad_mesh.element_size, quad_mesh.nr_of_elements);

	dx12core::GetDx12Core().GetDirectCommand()->Reset();
	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(vertex.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	dx12core::GetDx12Core().CreateRaytracingStructure(&vertex);

	dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(quad_mesh, vertex_2_transform);
	RayTracingObject quad_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject();

	dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(vertex, vertex_transform);
	RayTracingObject vertex1_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject();

	dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(vertex_2);
	RayTracingObject vertex2_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject();

	dx12core::GetDx12Core().GetRayObjectManager()->CreateScene({vertex1_ray_tracing_object, quad_ray_tracing_object, vertex2_ray_tracing_object});
	//dx12core::GetDx12Core().GetRayObjectManager()->CreateScene({ vertex2_ray_tracing_object });

	//dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(vertex_2);
	//RayTracingObject vertex2_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject();

	dx12raytracingrenderpipeline* raytracing_render_pipeline = new dx12raytracingrenderpipeline();
	//raytracing_render_pipeline->AddStructuredBuffer(0, D3D12_SHADER_VISIBILITY_ALL, false);//, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
	raytracing_render_pipeline->AddShaderResource(0, D3D12_SHADER_VISIBILITY_ALL, false);
	//raytracing_render_pipeline->AddConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL, false, D3D12_ROOT_PARAMETER_TYPE_SRV);
	raytracing_render_pipeline->AddUnorderedAccess(0, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->AddConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->CreateRayTracingStateObject("x64/Debug/RayTracingShaders.cso", L"ClosestHitShader", sizeof(RayPayloadData), 0);
	raytracing_render_pipeline->CreateShaderRecordBuffers(L"RayGenerationShader", L"MissShader");

	dx12core::GetDx12Core().GetDirectCommand()->Execute();
	dx12core::GetDx12Core().GetDirectCommand()->SignalAndWait();

	float rotation = 0;

	bool window_exist = true;
	while (window_exist)
	{
		window_exist = window.WinMsg();
		if (!window_exist)
			break;

		//Rasterizer
		dx12core::GetDx12Core().SetRenderPipeline(render_pipeline);
		dx12core::GetDx12Core().PreDraw();
		dx12core::GetDx12Core().Draw();

		rotation += 0.05f;
		//dx12core::GetDx12Core().GetRayObjectManager()->UpdateScene({ vertex1_ray_tracing_object, quad_ray_tracing_object, vertex2_ray_tracing_object });
		//dx12core::GetDx12Core().SetTopLevelTransform(rotation, vertex1_ray_tracing_object);

		//Raytracing
		dx12core::GetDx12Core().SetRayTracingRenderPipeline(raytracing_render_pipeline);
		dx12core::GetDx12Core().PreDispatchRays();
		dx12core::GetDx12Core().DispatchRays();


		dx12core::GetDx12Core().FinishDraw();
	}

	delete render_pipeline;
	delete raytracing_render_pipeline;

	//dx12core::GetDx12Core();

	return 0;
}