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
	SimpleVertex cube[36] =
	{
		{{-1.0f,-1.0f,-1.0f}, {0,0}}, // triangle 1 : begin
		{{-1.0f,-1.0f, 1.0f}, {0,0}},
		{{-1.0f, 1.0f, 1.0f}, {0,0}},

		{{1.0f, 1.0f,-1.0f}, {0,0}},
		{{-1.0f,-1.0f,-1.0f}, {0,0}},
		{{-1.0f, 1.0f,-1.0f}, {0,0}},

		{{1.0f,-1.0f, 1.0f}, {0,0}},
		{{-1.0f,-1.0f,-1.0f}, {0,0}},
		{{1.0f,-1.0f,-1.0f,}, {0,0}},

		{{1.0f, 1.0f,-1.0f}, {0,0}}, //f
		{{1.0f,-1.0f,-1.0f}, {0,0}}, //f
		{{-1.0f,-1.0f,-1.0f}, {0,0}}, //f

		{{-1.0f,-1.0f,-1.0f}, {0,0}}, //f
		{{-1.0f, 1.0f, 1.0f}, {0,0}}, //f
		{{-1.0f, 1.0f,-1.0f}, {0,0}}, //f

		{{1.0f,-1.0f, 1.0f}, {0,0}}, //f
		{{-1.0f,-1.0f, 1.0f}, {0,0}}, //f
		{{-1.0f,-1.0f,-1.0f}, {0,0}}, //f

		{{-1.0f, 1.0f, 1.0f}, {0,0}}, //f
		{{-1.0f,-1.0f, 1.0f}, {0,0}},//f
		{{1.0f,-1.0f, 1.0f}, {0,0}},//f

		{{1.0f, 1.0f, 1.0f}, {0,0}}, //f
		{{1.0f,-1.0f,-1.0f}, {0,0}}, //f
		{{1.0f, 1.0f,-1.0f}, {0,0}}, //f

		{{1.0f,-1.0f,-1.0f}, {0,0}}, //f
		{{1.0f, 1.0f, 1.0f}, {0,0}}, //f
		{{1.0f,-1.0f, 1.0f}, {0,0}}, //f

		{{1.0f, 1.0f, 1.0f}, {0,0}}, //f
		{{1.0f, 1.0f,-1.0f}, {0,0}}, //f
		{{-1.0f, 1.0f,-1.0f}, {0,0}}, //f

		{{1.0f, 1.0f, 1.0f}, {0,0}}, //f
		{{-1.0f, 1.0f,-1.0f}, {0,0}}, //f
		{{-1.0f, 1.0f, 1.0f}, {0,0}}, //f

		{{1.0f, 1.0f, 1.0f}, {0,0}},
		{{-1.0f, 1.0f, 1.0f}, {0,0}},
		{{1.0f,-1.0f, 1.0f}, {0,0}}
	};

	D3D12_RAYTRACING_AABB sphere_aabb;
	sphere_aabb.MinX = -1;
	sphere_aabb.MaxX = 1;
	sphere_aabb.MinY = -1;
	sphere_aabb.MaxY = 1;
	sphere_aabb.MinZ = -1;
	sphere_aabb.MaxZ = 1;

	BufferResource vertex = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(triangle, sizeof(SimpleVertex), 3, TextureType::TEXTURE_SRV);
	BufferResource vertex_2 = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(triangle_2, sizeof(SimpleVertex), 3, TextureType::TEXTURE_SRV);
	BufferResource quad_mesh = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(quad, sizeof(SimpleVertex), 6, TextureType::TEXTURE_SRV);
	BufferResource cube_mesh = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(cube, sizeof(SimpleVertex), 36, TextureType::TEXTURE_SRV);
	BufferResource sphere_aabb_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(&sphere_aabb, sizeof(D3D12_RAYTRACING_AABB), 1, TextureType::TEXTURE_SRV);

	float colours[24] = {1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f,
						0.0f, 1.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f,
						0.0f, 1.0f, 0.0f,
						1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, 1.0f,
										};

	BufferResource triangle_colours = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(&colours, 3 * sizeof(float), 8, TextureType::TEXTURE_SRV);

	DirectX::XMFLOAT3 camera_position = { 0.0f, 0.0f, -3.0f };

	DirectX::XMMATRIX view_matrix = DirectX::XMMatrixLookAtLH({camera_position.x, camera_position.y, camera_position.z, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f});
	float fov = DirectX::XM_PIDIV4;
	DirectX::XMMATRIX projection_matrix = DirectX::XMMatrixPerspectiveFovLH(fov, window.GetWindowWidth() / window.GetWindowHeight(), 1.0f, 125);
	DirectX::XMMATRIX view_projection_inverse_matrix = view_matrix * projection_matrix;

	DirectX::XMVECTOR det;
	view_projection_inverse_matrix = DirectX::XMMatrixInverse(&det, view_projection_inverse_matrix);
	DirectX::XMMATRIX view_inverse_matrix = DirectX::XMMatrixInverse(&det, view_matrix);
	DirectX::XMMATRIX projection_inverse_matrix = DirectX::XMMatrixInverse(&det, projection_matrix);

	struct ViewProjectionMatrix
	{
		DirectX::XMMATRIX view_projection;
		DirectX::XMMATRIX view_inverse;
		DirectX::XMMATRIX projection_inverse;
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
		DirectX::XMFLOAT3 camera_position;
		float max_recursion;
	};
	ViewProjectionMatrix camera_data;
	camera_data.view_projection = view_projection_inverse_matrix;
	camera_data.projection_inverse = projection_inverse_matrix;
	camera_data.view_inverse = view_inverse_matrix;
	camera_data.projection = projection_matrix;
	camera_data.view = view_matrix;
	camera_data.camera_position = camera_position;
	camera_data.max_recursion = 4;
	//camera_data.camera_position[0] = camera_position[0]; camera_data.camera_position[1] = camera_position[1]; camera_data.camera_position[2] = camera_position[2];

	BufferResource view_projection_matrix = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&camera_data), sizeof(ViewProjectionMatrix), 1);

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

	std::vector<RayTracingObject> ray_tracing_objects;

	int num_cubes = 1;
	for (int i = 0; i < num_cubes; ++i)
	{
		DirectX::XMFLOAT3X4 transform;
		DirectX::XMMATRIX XMMATRIX_transform = DirectX::XMMatrixRotationRollPitchYaw(0,1,10) * DirectX::XMMatrixTranslation(-1.0f, 0.0f, 4.0f);
		//XMMATRIX_transform = XMMATRIX_transform * camera_to_world * projection_matrix;
		DirectX::XMStoreFloat3x4(&transform, XMMATRIX_transform);

		dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(cube_mesh);
		RayTracingObject cube_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject(0, transform);

		ray_tracing_objects.push_back(cube_ray_tracing_object);
	}

	std::vector<SphereAABB> sphere_aabb_positions(10);// = { {1.f,0,2}, 1.0f };
	sphere_aabb_positions[0] = { {1.f,0,2}, 1.0f };
	sphere_aabb_positions[1] = { {-1.5f,0,0.5f}, 0.85f };
	BufferResource sphere_aabb_position_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(sphere_aabb_positions.data(), sizeof(SphereAABB), 10, TextureType::TEXTURE_SRV);
	//BufferResource sphere_aabb_position_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(sizeof(SphereAABB) * 10, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, TextureType::TEXTURE_SRV);

	DirectX::XMFLOAT3X4 transform_sphere;
	DirectX::XMMATRIX XMMATRIX_transform_sphere = DirectX::XMMatrixRotationRollPitchYaw(0, 0, 0) * 
		DirectX::XMMatrixTranslation(sphere_aabb_positions[0].position.x, sphere_aabb_positions[0].position.y, sphere_aabb_positions[0].position.z);
	
	DirectX::XMStoreFloat3x4(&transform_sphere, XMMATRIX_transform_sphere);
	dx12core::GetDx12Core().GetRayObjectManager()->AddAABB(sphere_aabb_buffer);
	RayTracingObject sphere_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObjectAABB(1, transform_sphere);


	XMMATRIX_transform_sphere = DirectX::XMMatrixRotationRollPitchYaw(0, 0, 0) *
		DirectX::XMMatrixTranslation(sphere_aabb_positions[1].position.x, sphere_aabb_positions[1].position.y, sphere_aabb_positions[1].position.z);
	DirectX::XMStoreFloat3x4(&transform_sphere, XMMATRIX_transform_sphere);

	RayTracingObject srto_2 = dx12core::GetDx12Core().GetRayObjectManager()->CopyRayTracingObjectAABB(sphere_ray_tracing_object, transform_sphere);;
	ray_tracing_objects.push_back(sphere_ray_tracing_object);
	ray_tracing_objects.push_back(srto_2);

	dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(vertex);
	RayTracingObject vertex1_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject(0, transform);

	dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(vertex_2);
	RayTracingObject vertex2_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject(0, transform_2);

	ray_tracing_objects.push_back(vertex1_ray_tracing_object);
	//ray_tracing_objects.push_back(vertex2_ray_tracing_object);

	//std::vector<UINT> indexes(ray_tracing_objects.size());

	dx12core::GetDx12Core().GetRayObjectManager()->CreateScene(ray_tracing_objects); //, quad_2_ray_tracing_object, quad_3_ray_tracing_object, quad_4_ray_tracing_object, quad_5_ray_tracing_object, quad_6_ray_tracing_object });
	//dx12core::GetDx12Core().GetRayObjectManager()->CreateScene({ vertex2_ray_tracing_object });

	//dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(vertex_2);
	//RayTracingObject vertex2_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject();

	dx12raytracingrenderpipeline* raytracing_render_pipeline = new dx12raytracingrenderpipeline();
	//raytracing_render_pipeline->AddStructuredBuffer(0, D3D12_SHADER_VISIBILITY_ALL, false);//, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
	raytracing_render_pipeline->AddShaderResource(0, D3D12_SHADER_VISIBILITY_ALL, false);
	//raytracing_render_pipeline->AddConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL, false, D3D12_ROOT_PARAMETER_TYPE_SRV);
	raytracing_render_pipeline->AddUnorderedAccess(0, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->AddStructuredBuffer(1, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->AddConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->AddStructuredBuffer(2, D3D12_SHADER_VISIBILITY_ALL, false);
	//raytracing_render_pipeline->AddConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->CreateRayTracingStateObject("x64/Debug/RayTracingShaders.cso", L"ClosestHitShader", sizeof(RayPayloadData), camera_data.max_recursion - 1);
	raytracing_render_pipeline->CreateShaderRecordBuffers(L"RayGenerationShader", L"MissShader", triangle_colours, view_projection_matrix, sphere_aabb_position_buffer);

	dx12core::GetDx12Core().GetDirectCommand()->Execute();
	dx12core::GetDx12Core().GetDirectCommand()->SignalAndWait();

	float rotation = 0;

	DirectX::XMFLOAT3 camera_direction = {0.0f, 0.0f, 1.0f};
	DirectX::XMVECTOR vector_camera_position;
	DirectX::XMVECTOR vector_camera_direction;
	DirectX::XMVECTOR vector_up = { 0.0f, 1.0f, 0.0f, 0.0f };
	DirectX::XMVECTOR vector_result = {};

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

		//rotation += 0.0005f;

		vector_camera_position = DirectX::XMLoadFloat3(&camera_position);
		vector_camera_direction = DirectX::XMLoadFloat3(&camera_direction);

		if (Window::s_window_key_inputs.a_key)
		{
			vector_result = DirectX::XMVector3Cross(vector_camera_direction, vector_up);
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_result, -0.005f));
		}
		if (Window::s_window_key_inputs.d_key)
		{
			vector_result = DirectX::XMVector3Cross(vector_camera_direction, vector_up);
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_result, 0.005f));
		}
		if (Window::s_window_key_inputs.w_key)
		{
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_camera_direction, -0.005f));
		}
		if (Window::s_window_key_inputs.s_key)
		{
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_camera_direction, 0.005f));
		}
		if (Window::s_window_key_inputs.shift_key)
		{
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_up, -0.005f));
		}
		if (Window::s_window_key_inputs.left_control_key)
		{
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_up, 0.005f));
		}
		if (Window::s_window_key_inputs.q_key)
		{
			vector_result = DirectX::XMVector3Cross(vector_camera_direction, vector_up);
			vector_result = DirectX::XMVectorSubtract(vector_camera_direction, DirectX::XMVectorScale(vector_result, -0.005f));
			vector_camera_direction = DirectX::XMVector3Normalize(vector_result);
		}
		if (Window::s_window_key_inputs.e_key)
		{
			vector_result = DirectX::XMVector3Cross(vector_camera_direction, vector_up);
			vector_result = DirectX::XMVectorSubtract(vector_camera_direction, DirectX::XMVectorScale(vector_result, 0.005f));
			vector_camera_direction = DirectX::XMVector3Normalize(vector_result);
		}

		DirectX::XMStoreFloat3(&camera_position, vector_camera_position);
		DirectX::XMStoreFloat3(&camera_direction, vector_camera_direction);

		//f = DirectX::XMVector3Normalize(f);

		view_matrix = DirectX::XMMatrixLookAtLH({ camera_position.x, camera_position.y, camera_position.z, 0.0f },
			{ camera_position.x + camera_direction.x, camera_position.y + camera_direction.y, camera_position.z + camera_direction.z, 0.0f},
			vector_up);
		//camera_to_world *= projection_matrix;
		//camera_to_world = DirectX::XMMatrixInverse(nullptr, camera_to_world);
		//camera_data.view_projection = camera_to_world;
		camera_data.view = view_matrix;
		camera_data.view_inverse = DirectX::XMMatrixInverse(&det, view_matrix);;
		camera_data.camera_position = { camera_position.x, camera_position.y, camera_position.z};
		dx12core::GetDx12Core().GetBufferManager()->UpdateBuffer(view_projection_matrix, &camera_data);

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