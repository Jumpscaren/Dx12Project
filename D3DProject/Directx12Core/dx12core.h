#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>
#include <vector>
#include "dx12command.h"
#include "dx12texturemanager.h"
#include "dx12renderpipeline.h"
#include "dx12buffermanager.h"
#include "dx12rayobjectmanager.h"
#include "dx12raytracingrenderpipeline.h"

class dx12core
{
private:
	Microsoft::WRL::ComPtr<IDXGIFactory5> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<ID3D12Device5> m_device;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapchain;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dh_RTV;
	UINT m_dh_RTV_current_offset;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dh_DSV;
	UINT m_dh_DSV_current_offset;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_direct_command_queue;
	dx12command* m_direct_command;
	
	UINT m_backbuffer_count;
	UINT m_backbuffer_width, m_backbuffer_height;
	std::vector<TextureResource> m_backbuffers;

	dx12texturemanager* m_texture_manager;

	TextureResource m_depth_stencil;

	dx12renderpipeline* m_render_pipeline;
	dx12raytracingrenderpipeline* m_ray_tracing_render_pipeline;

	dx12buffermanager* m_buffer_manager;

	dx12rayobjectmanager* m_ray_object_manager;

	//const UINT queries = 1;
	//Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_query_timestamp;
	//BufferResource m_buffer_query_data;

//Raytracing
private:
	TextureResource m_output_uav;

private:
	static dx12core s_instance;
private:
	void EnableDebugLayer();
	void EnableGPUBasedValidation();
	bool CheckDXRSupport(ID3D12Device* device);
	void CreateDevice();
	void CreateSwapchain(HWND window_handle);
	void CreateCommandQueue(ID3D12CommandQueue** command_queue, const D3D12_COMMAND_LIST_TYPE& command_type);
	void CreateQuery();
	dx12core();
public:
	~dx12core();
	dx12core(const dx12core& other) = delete;
	dx12core& operator=(const dx12core& other) = delete;
	static dx12core& GetDx12Core();
	void Init(HWND hwnd, UINT backbuffer_count = 2);
	ID3D12Device5* GetDevice();
	IDXGISwapChain3* GetSwapChain();
	dx12command* GetDirectCommand();
	dx12texturemanager* GetTextureManager();
	dx12buffermanager* GetBufferManager();
	ID3D12CommandQueue* GetCommandQueue();
	dx12rayobjectmanager* GetRayObjectManager();
	void SetRenderPipeline(dx12renderpipeline* render_pipeline);
	void SetRayTracingRenderPipeline(dx12raytracingrenderpipeline* render_pipeline);
	void Show();
	void PreDraw();
	void Draw();
	void FinishDraw();

//Raytracing
public:
	void CreateRaytracingStructure(BufferResource* vertex_buffer);
	TextureResource GetOutputUAV();
	void PreDispatchRays();
	void DispatchRays();
	void AfterDispatchRays();
	void SetTopLevelTransform(float rotation, const RayTracingObject& acceleration_structure_object);
};
