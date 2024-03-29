#include <iostream>
#include "Window.h"
#include "Directx12Core/dx12core.h"
#include "RayTracingShaderHeader.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#include <chrono>

int main()
{
	std::cout << "Hello World\n";

	//Window window(2560, 1440, L"Project", L"Project");

	//Window window(3840, 2160, L"Project", L"Project");

	//Window window(1920, 1080, L"Project", L"Project");

	Window window(1280, 720, L"Project", L"Project");
	
	dx12core::GetDx12Core().Init(window.GetWindowHandle(), 2);

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	auto descriptor_heap = dx12core::GetDx12Core().GetTextureManager()->GetShaderBindableDescriptorHeap();
	ImGui_ImplWin32_Init(window.GetWindowHandle());
	auto imgui_cpu_handle = descriptor_heap->GetCPUDescriptorHandleForHeapStart();
	auto imgui_gpu_handle = descriptor_heap->GetGPUDescriptorHandleForHeapStart();
	dx12core::GetDx12Core().GetTextureManager()->IncreaseShaderBinableDescriptorHeap();

	ImGui_ImplDX12_Init(dx12core::GetDx12Core().GetDevice(), 1,
		DXGI_FORMAT_R8G8B8A8_UNORM, descriptor_heap,
		imgui_cpu_handle,
		imgui_gpu_handle);

	TCHAR pwd[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, pwd);
	std::wstring gf(pwd);
	std::string of(gf.begin(), gf.end());
	std::cout << of.c_str() << "\n";

	TextureResource texture = dx12core::GetDx12Core().GetTextureManager()->CreateTexture2D("Textures/image.png", TextureType::TEXTURE_SRV);

	struct Vertex
	{
		float position[3];
		float uv[2];
	};
	Vertex triangle[3] =
	{
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},
		{{0.0f, 0.5f, 0.0f}, {0.5f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}}
	};
	Vertex triangle_2[3] =
	{
		{{-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f}, {0.5f, 0.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}}
	};
	Vertex quad[6] =
	{
		{{-0.75f, -0.75f, 0.5f}, {0.0f, 1.0f}},
		{{-0.75f, 0.75f, 0.5f}, {0.0f, 0.0f}},
		{{0.75f, 0.75f, 0.5f}, {1.0f, 0.0f}},

		{{-0.75f, -0.75f, 0.5f}, {0.0f, 1.0f}},
		{{0.75f, 0.75f, 0.5f}, {1.0f, 0.0f}},
		{{0.75f, -0.75f, 0.5f}, {1.0f, 1.0f}},
	};
	Vertex cube[36] =
	{
		{{-1.0f,-1.0f,-1.0f}, {0,0}}, 
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

	BufferResource vertex = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(triangle, sizeof(Vertex), 3, TextureType::TEXTURE_SRV);
	BufferResource vertex_2 = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(triangle_2, sizeof(Vertex), 3, TextureType::TEXTURE_SRV);
	BufferResource quad_mesh = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(quad, sizeof(Vertex), 6, TextureType::TEXTURE_SRV);
	BufferResource cube_mesh = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(cube, sizeof(Vertex), 36, TextureType::TEXTURE_SRV);
	BufferResource sphere_aabb_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(&sphere_aabb, sizeof(D3D12_RAYTRACING_AABB), 1, TextureType::TEXTURE_SRV);

	TriangleColour colours[3] = { {{1.0f, 0.0f, 0.0f}}, {{0.0f, 0.0f, 1.0f}}, {{0.0f, 1.0f, 0.0f}} };

	BufferResource triangle_colours = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(&colours, sizeof(colours), 3, TextureType::TEXTURE_SRV);


	//Set up the camera
	//DirectX::XMFLOAT3 camera_position = { -20.0f, 10.0f, 10.0f };
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
		DirectX::XMMATRIX view_inverse;
		DirectX::XMMATRIX projection_inverse;
		DirectX::XMFLOAT3 camera_position;
		float max_recursion;
	};
	ViewProjectionMatrix camera_data;
	camera_data.projection_inverse = projection_inverse_matrix;
	camera_data.view_inverse = view_inverse_matrix;
	camera_data.camera_position = camera_position;
	camera_data.max_recursion = 5;

	BufferResource view_projection_matrix = dx12core::GetDx12Core().GetBufferManager()->CreateBuffer((void*)(&camera_data), sizeof(ViewProjectionMatrix), 1);

	DirectX::XMFLOAT3X4 transform;
	DirectX::XMStoreFloat3x4(&transform, DirectX::XMMatrixTranslation(0,0,0));
	DirectX::XMFLOAT3X4 transform_2;
	DirectX::XMMATRIX transform_matrix_2 = DirectX::XMMatrixTranslation(0, 0.5f, 0) * DirectX::XMMatrixRotationZ(10);
	DirectX::XMStoreFloat3x4(&transform_2, transform_matrix_2);

	dx12core::GetDx12Core().GetDirectCommand()->Execute();
	dx12core::GetDx12Core().GetDirectCommand()->SignalAndWait();

	//Create rasterizer pipeline
	dx12renderpipeline* render_pipeline = new dx12renderpipeline();
	render_pipeline->AddStructuredBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX, false);
	render_pipeline->AddShaderResource(0, D3D12_SHADER_VISIBILITY_PIXEL, false);
	render_pipeline->AddStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, D3D12_SHADER_VISIBILITY_PIXEL, false);
	render_pipeline->CreateRenderPipeline("x64/Debug/VertexShader1.cso", "x64/Debug/PixelShader1.cso");
	//Create renderobject for the quad
	auto object = render_pipeline->CreateRenderObject();
	object->AddStructuredBuffer(quad_mesh, 0, D3D12_SHADER_VISIBILITY_VERTEX, false);
	object->AddShaderResourceView(texture, 0, D3D12_SHADER_VISIBILITY_PIXEL, false);
	object->SetMeshData(quad_mesh.element_size, quad_mesh.nr_of_elements);

	dx12core::GetDx12Core().GetDirectCommand()->Reset();
	dx12core::GetDx12Core().GetDirectCommand()->TransistionBuffer(vertex.buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	dx12core::GetDx12Core().CreateOutputTexture();

	//Raytracing objects for the scene (TLAS)
	std::vector<RayTracingObject> ray_tracing_objects;

	//Create cubes
	int num_cubes = 10;
	for (int i = 0; i < num_cubes; ++i)
	{
		for (int j = 0; j < num_cubes; ++j)
		{
			DirectX::XMFLOAT3X4 transform;
			DirectX::XMMATRIX XMMATRIX_transform = DirectX::XMMatrixRotationRollPitchYaw(i, 1 + i, 10 + i) * DirectX::XMMatrixTranslation(-20.0f + i * 8.0f, sin(i) * 2.0f, -20.0f + j * 8.0f);

			DirectX::XMStoreFloat3x4(&transform, XMMATRIX_transform);

			dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(cube_mesh);
			RayTracingObject cube_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject(0, transform, (i+j) % 3);

			ray_tracing_objects.push_back(cube_ray_tracing_object);
		}
	}

	//Create spheres
	std::vector<SphereAABB> sphere_aabb_positions(2);
	sphere_aabb_positions[0] = { {1.f,0,2}, 1.0f };
	sphere_aabb_positions[1] = { {-1.5f,0,0.5f}, 0.85f };

	DirectX::XMFLOAT3X4 transform_sphere;
	DirectX::XMMATRIX XMMATRIX_transform_sphere = DirectX::XMMatrixRotationRollPitchYaw(0, 0, 0) * 
		DirectX::XMMatrixTranslation(sphere_aabb_positions[0].position.x, sphere_aabb_positions[0].position.y, sphere_aabb_positions[0].position.z);
	
	DirectX::XMStoreFloat3x4(&transform_sphere, XMMATRIX_transform_sphere);
	dx12core::GetDx12Core().GetRayObjectManager()->AddAABB(sphere_aabb_buffer);
	RayTracingObject sphere_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObjectAABB(1, transform_sphere, 0);


	DirectX::XMMATRIX XMMATRIX_transform_sphere_2 = DirectX::XMMatrixRotationRollPitchYaw(0, 0, 0) *
		DirectX::XMMatrixTranslation(sphere_aabb_positions[1].position.x, sphere_aabb_positions[1].position.y, sphere_aabb_positions[1].position.z);
	DirectX::XMStoreFloat3x4(&transform_sphere, XMMATRIX_transform_sphere_2);

	RayTracingObject srto_2 = dx12core::GetDx12Core().GetRayObjectManager()->CopyRayTracingObjectAABB(sphere_ray_tracing_object, transform_sphere, 1);
	ray_tracing_objects.push_back(sphere_ray_tracing_object);
	ray_tracing_objects.push_back(srto_2);
	int sphere_index = ray_tracing_objects.size()-1;

	int num_spheres = 15;
	for (int i = 0; i < num_spheres; ++i)
	{
		for (int j = 0; j < num_spheres; ++j)
		{
			for (int k = 0; k < num_spheres; ++k)
			{
				sphere_aabb_positions.push_back({ {1.0f - (j + 1) * 2.2f, k * 2.2f, 0.5f + (i + 1) * 2.2f}, 0.5f });
				int index = sphere_aabb_positions.size() - 1;

				DirectX::XMMATRIX XMMATRIX_transform_sphere_2 = DirectX::XMMatrixRotationRollPitchYaw(0, 0, 0) *
					DirectX::XMMatrixTranslation(sphere_aabb_positions[index].position.x, sphere_aabb_positions[index].position.y, sphere_aabb_positions[index].position.z);
				DirectX::XMStoreFloat3x4(&transform_sphere, XMMATRIX_transform_sphere_2);
				RayTracingObject srto = dx12core::GetDx12Core().GetRayObjectManager()->CopyRayTracingObjectAABB(sphere_ray_tracing_object, transform_sphere, index);

				ray_tracing_objects.push_back(srto);
			}
		}
	}

	BufferResource sphere_aabb_position_buffer = dx12core::GetDx12Core().GetBufferManager()->CreateStructuredBuffer(sphere_aabb_positions.data(), sizeof(SphereAABB), sphere_aabb_positions.size(), TextureType::TEXTURE_SRV);

	dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(vertex);
	RayTracingObject vertex1_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject(0, transform, 1);

	dx12core::GetDx12Core().GetRayObjectManager()->AddMesh(vertex_2);
	RayTracingObject vertex2_ray_tracing_object = dx12core::GetDx12Core().GetRayObjectManager()->CreateRayTracingObject(0, transform_2, 2);

	ray_tracing_objects.push_back(vertex1_ray_tracing_object);
	ray_tracing_objects.push_back(vertex2_ray_tracing_object);


	dx12core::GetDx12Core().GetRayObjectManager()->CreateScene(ray_tracing_objects);

	//Setup the root signature and create the stateobject
	dx12raytracingrenderpipeline* raytracing_render_pipeline = new dx12raytracingrenderpipeline();
	raytracing_render_pipeline->AddShaderResource(0, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->AddUnorderedAccess(0, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->AddStructuredBuffer(1, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->AddConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->AddStructuredBuffer(2, D3D12_SHADER_VISIBILITY_ALL, false);
	raytracing_render_pipeline->AddStructuredBuffer(3, D3D12_SHADER_VISIBILITY_ALL, false);

	std::vector<HitGroupInfo> hit_groups(2);
	hit_groups[0].hit_group_name = L"HitGroup"; hit_groups[0].closest_hit_shader_name = L"ClosestHitShader"; hit_groups[0].hit_type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hit_groups[1].hit_group_name = L"ReflectionHitGroup"; hit_groups[1].closest_hit_shader_name = L"ReflectionClosestHitShader"; hit_groups[1].hit_type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
	hit_groups[1].intersection_shader_name = L"ReflectionIntersectionShader";
	raytracing_render_pipeline->CreateRayTracingStateObject("x64/Debug/RayTracingShaders.cso", hit_groups, sizeof(RayPayloadData), sizeof(SphereNormal), camera_data.max_recursion - 1);
	raytracing_render_pipeline->CreateShaderRecordBuffers(L"RayGenerationShader", L"MissShader", triangle_colours, view_projection_matrix, sphere_aabb_position_buffer, hit_groups);

	dx12core::GetDx12Core().GetDirectCommand()->Execute();
	dx12core::GetDx12Core().GetDirectCommand()->SignalAndWait();

	float rotation = 0;

	DirectX::XMFLOAT3 camera_direction = {0.0f, 0.0f, 1.0f};
	DirectX::XMVECTOR vector_camera_position;
	DirectX::XMVECTOR vector_camera_direction;
	DirectX::XMVECTOR vector_up = { 0.0f, 1.0f, 0.0f, 0.0f };
	DirectX::XMVECTOR vector_result = {};

	bool window_exist = true;
	float rotation_speed = 0.0015f;
	float fly_speed = 0.004f;

	std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
	long double duration = 0;

	//For the performance test
	const long double max_duration = 15000.0f;
	long double total_duration = 0;
	long double total_frames = 0;

	while (window_exist)
	{
		start = std::chrono::high_resolution_clock::now();

		window_exist = window.WinMsg();
		if (!window_exist)
			break;

		//Rasterizer
		dx12core::GetDx12Core().SetRenderPipeline(render_pipeline);
		dx12core::GetDx12Core().PreDraw();
		dx12core::GetDx12Core().Draw();

		vector_camera_position = DirectX::XMLoadFloat3(&camera_position);
		vector_camera_direction = DirectX::XMLoadFloat3(&camera_direction);

		float new_fly_speed = duration * fly_speed;
		float new_rotation_speed = duration * rotation_speed;

		if (Window::s_window_key_inputs.a_key)
		{
			vector_result = DirectX::XMVector3Cross(vector_camera_direction, vector_up);
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_result, -new_fly_speed));
		}
		if (Window::s_window_key_inputs.d_key)
		{
			vector_result = DirectX::XMVector3Cross(vector_camera_direction, vector_up);
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_result, new_fly_speed));
		}
		if (Window::s_window_key_inputs.w_key)
		{
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_camera_direction, -new_fly_speed));
		}
		if (Window::s_window_key_inputs.s_key)
		{
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_camera_direction, new_fly_speed));
		}
		if (Window::s_window_key_inputs.shift_key)
		{
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_up, -new_fly_speed));
		}
		if (Window::s_window_key_inputs.left_control_key)
		{
			vector_camera_position = DirectX::XMVectorSubtract(vector_camera_position, DirectX::XMVectorScale(vector_up, new_fly_speed));
		}
		if (Window::s_window_key_inputs.q_key)
		{
			vector_result = DirectX::XMVector3Cross(vector_camera_direction, vector_up);
			vector_result = DirectX::XMVectorSubtract(vector_camera_direction, DirectX::XMVectorScale(vector_result, -new_rotation_speed));
			vector_camera_direction = DirectX::XMVector3Normalize(vector_result);
		}
		if (Window::s_window_key_inputs.e_key)
		{
			vector_result = DirectX::XMVector3Cross(vector_camera_direction, vector_up);
			vector_result = DirectX::XMVectorSubtract(vector_camera_direction, DirectX::XMVectorScale(vector_result, new_rotation_speed));
			vector_camera_direction = DirectX::XMVector3Normalize(vector_result);
		}


		//Update the camera
		DirectX::XMStoreFloat3(&camera_position, vector_camera_position);
		DirectX::XMStoreFloat3(&camera_direction, vector_camera_direction);
		view_matrix = DirectX::XMMatrixLookAtLH({ camera_position.x, camera_position.y, camera_position.z, 0.0f },
			{ camera_position.x + camera_direction.x, camera_position.y + camera_direction.y, camera_position.z + camera_direction.z, 0.0f},
			vector_up);

		camera_data.view_inverse = DirectX::XMMatrixInverse(&det, view_matrix);
		camera_data.camera_position = { camera_position.x, camera_position.y, camera_position.z};
		dx12core::GetDx12Core().GetBufferManager()->UpdateBuffer(view_projection_matrix, &camera_data);


		//Move the sphere
		rotation += 0.005f;
		sphere_aabb_positions[1].position.x = sin(rotation) * 4;
		sphere_aabb_positions[1].position.z = cos(rotation) * 4;
		XMMATRIX_transform_sphere_2 = DirectX::XMMatrixRotationRollPitchYaw(0, 0, 0) *
			DirectX::XMMatrixTranslation(sphere_aabb_positions[1].position.x, sphere_aabb_positions[1].position.y, sphere_aabb_positions[1].position.z);
		DirectX::XMStoreFloat3x4(&ray_tracing_objects[sphere_index].instance_transform, XMMATRIX_transform_sphere_2);
		dx12core::GetDx12Core().GetBufferManager()->UpdateBuffer(sphere_aabb_position_buffer, sphere_aabb_positions.data());
		dx12core::GetDx12Core().GetRayObjectManager()->UpdateScene(ray_tracing_objects);

		//Raytracing
		dx12core::GetDx12Core().SetRayTracingRenderPipeline(raytracing_render_pipeline);
		dx12core::GetDx12Core().PreDispatchRays();
		dx12core::GetDx12Core().DispatchRays();
		dx12core::GetDx12Core().AfterDispatchRays();

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("App Statistics");
		{
			ImGui::Text("%f ms", duration);
		}
		ImGui::End();

		ImGui::Render();

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx12core::GetDx12Core().GetDirectCommand()->GetCommandList());

		dx12core::GetDx12Core().FinishDraw();

		end = std::chrono::high_resolution_clock::now();

		duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() * 1e-3;
		++total_frames;
		total_duration += duration;
		//if (total_duration >= max_duration)
		//	break;
	}

	std::cout << "Frames: " << total_frames << "\n";
	std::cout << "Total Duration: " << total_duration << "\n";
	std::cout << "Average Duration: " << total_duration / total_frames << "\n";

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete render_pipeline;
	delete raytracing_render_pipeline;

	return 0;
}