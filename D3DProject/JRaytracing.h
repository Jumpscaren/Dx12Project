#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <assert.h>

struct SimpleVertex
{
	float position[3];
	float uv[2];
};

template<size_t RootArgumentsByteSize>
struct ShaderRecord
{
	unsigned char shaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES]; //Not a string, but data for a 'pointer' to the shader
	unsigned char rootArguments[RootArgumentsByteSize];
};

template<size_t RootArgumentsByteSize>
struct RayGenerationShaderRecord
{
	ID3D12Resource* buffer = nullptr;

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE GetGpuAdressRange() const
	{
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE toReturn;
		toReturn.StartAddress = buffer->GetGPUVirtualAddress();
		toReturn.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			+ RootArgumentsByteSize;

		return toReturn;
	}
};

template<size_t RootArgumentsByteSize, short NrOfRecords>
struct ShaderTable
{
	ID3D12Resource* buffer = nullptr;

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetGpuAdressRangeAndStride() const
	{
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE toReturn;
		toReturn.StartAddress = buffer->GetGPUVirtualAddress();
		toReturn.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			+ RootArgumentsByteSize * NrOfRecords;
		toReturn.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			+ RootArgumentsByteSize;

		return toReturn;
	}
};

class JRaytracing
{
public:
	JRaytracing() = default;
	~JRaytracing() = default;

	void setup(HWND hwnd);
	void run();

public:
	bool CheckSupportDXR(ID3D12Device* device);
	void ThrowIfFailed(HRESULT hr, const std::exception& exception);
	void EnableDebugLayer();
	void EnableGPUBasedValidation();
	IDXGIFactory* CreateFactory(UINT flags = 0);
	IDXGIAdapter* GetD3D12Adapter(IDXGIFactory* factory);
	ID3D12CommandQueue* CreateDirectCommandQueue(ID3D12Device* device);
	ID3D12CommandAllocator* CreateDirectCommandAllocator(ID3D12Device* device);
	ID3D12GraphicsCommandList* CreateDirectCommandList(ID3D12Device* device, ID3D12CommandAllocator* directAllocator);
	ID3D12Fence* CreateFence(ID3D12Device* device, UINT64 initialValue = 0);
	ID3D12Heap* CreateVertexHeap(ID3D12Device* device, size_t nrOfVertices);
	ID3D12Resource* CreateVertexBuffer(ID3D12Device* device, ID3D12Heap* heapToAllocateIn,
		size_t heapOffset, size_t nrOfVertices);
	ID3D12Heap* CreateUploadHeap(ID3D12Device* device, size_t sizeInBytes);
	ID3D12Resource* CreateUploadBuffer(ID3D12Device* device, ID3D12Heap* heapToAllocateIn,
		size_t heapOffset, size_t sizeInBytes);
	void TransitionResource(ID3D12Resource* resource, ID3D12GraphicsCommandList* commandList,
		D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_STATES newState);
	unsigned char* CreateTextureData(unsigned int dimension, unsigned short mipLevels, unsigned int& totalByteSize);
	ID3D12Resource* CreateTexture2DResource(ID3D12Device* device, unsigned int width,
		unsigned int height, unsigned int mipLevels = 1, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, D3D12_CLEAR_VALUE* clearColour = nullptr);
	size_t AlignAdress(size_t adress, size_t alignment);
	size_t MemcpyUploadData(unsigned char* mappedPtr, unsigned char* data, size_t uploadOffset, unsigned int alignment,
		UINT nrOfRows, UINT64 rowSizeInBytes, UINT rowPitch);
	size_t UploadData(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12Resource* uploadBuffer,
		ID3D12Resource* targetResource, unsigned char* data, size_t uploadOffset, unsigned int alignment,
		unsigned int subresource = 0);
	void ExecuteCommandList(ID3D12GraphicsCommandList* list, ID3D12CommandQueue* queue);
	void FlushCommandQueue(ID3D12Fence* fence, size_t& currentFenceValue, ID3D12CommandQueue* queue);
	ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
		UINT nrOfDescriptors, bool shaderVisible);
	void CreateTexture2DSRV(ID3D12Device* device, ID3D12Resource* texture, ID3D12DescriptorHeap* heap,
		UINT descriptorSize, UINT descriptorOffset);
	void CreateTexture2DDSV(ID3D12Device* device, ID3D12Resource* texture, ID3D12DescriptorHeap* heap,
		UINT descriptorSize, UINT descriptorOffset);
	IDXGISwapChain1* CreateSwapChain(IDXGIFactory2* factory, ID3D12CommandQueue* queue, HWND window,
		unsigned int windowWidth, unsigned int windowHeight, unsigned int bufferCount);
	void CreateBackbufferRTVs(ID3D12Device* device, IDXGISwapChain* swapChain, unsigned int backbufferCount,
		ID3D12DescriptorHeap* descriptorHeap, UINT descriptorSize, UINT descriptorStartOffset,
		std::vector<ID3D12Resource*>& backbufferResources);
	D3D12_ROOT_PARAMETER CreateRootDescriptor(D3D12_ROOT_PARAMETER_TYPE typeOfView,
		D3D12_SHADER_VISIBILITY visibleShader, UINT shaderRegister, UINT registerSpace = 0);
	D3D12_DESCRIPTOR_RANGE CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type,
		UINT nrOfDescriptors, UINT baseShaderRegister, UINT registerSpace = 0);
	D3D12_ROOT_PARAMETER CreateDescriptorTable(const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges,
		D3D12_SHADER_VISIBILITY visibleShader);
	D3D12_STATIC_SAMPLER_DESC CreateStaticSampler(D3D12_FILTER filter, UINT shaderRegister,
		D3D12_SHADER_VISIBILITY visibleShader = D3D12_SHADER_VISIBILITY_PIXEL, UINT registerSpace = 0);
	ID3D12RootSignature* CreateRootSignature(ID3D12Device* device,
		const std::vector<D3D12_ROOT_PARAMETER>& rootParameters,
		const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers = {},
		D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
	ID3DBlob* LoadCSO(const std::string& filepath);
	D3D12_RASTERIZER_DESC CreateRasterizerDesc();
	D3D12_RENDER_TARGET_BLEND_DESC CreateBlendDesc();
	D3D12_DEPTH_STENCIL_DESC CreateDepthStencilDesc();
	D3D12_STREAM_OUTPUT_DESC CreateStreamOutputDesc();
	ID3D12PipelineState* CreateGraphicsPipelineState(ID3D12Device* device,
		ID3D12RootSignature* rootSignature, const std::array<std::string, 5>& shaderPaths,
		DXGI_FORMAT dsvFormat, const std::vector<DXGI_FORMAT>& rtvFormats);
	void ResetCommandMemory(ID3D12CommandAllocator* allocator, ID3D12GraphicsCommandList* commandList);
	void PreDraw(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* shaderBindHeap,
		ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineState,
		ID3D12Resource* backbuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, unsigned int width, unsigned int height);
	void Draw(ID3D12GraphicsCommandList* commandList, ID3D12Resource* vertexBuffer,
		ID3D12DescriptorHeap* shaderBindableHeap, unsigned int vertexCount);
	void FinishFrame(ID3D12GraphicsCommandList* commandList, ID3D12Resource* backbuffer);
	bool CreateCommitedBuffer(ID3D12Device* device, ID3D12Resource*& toSet,
		D3D12_HEAP_TYPE heapType, size_t bufferSize, D3D12_RESOURCE_FLAGS flags,
		D3D12_RESOURCE_STATES initialState);
	bool BuildAccelerationStructureHelper(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* list,
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs,
		UINT64 resultBufferSize, ID3D12Resource*& resultBuffer,
		UINT64 scratchBufferSize, ID3D12Resource*& scratchBuffer);
	bool BuildBottomLevelAccelerationStructure(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* list, D3D12_GPU_VIRTUAL_ADDRESS vertexBufferFirstPosition,
		ID3D12Resource*& resultAccelerationStructureBuffer,
		ID3D12Resource*& scratchAccelerationStructureBuffer);
	bool BuildTopLevelAccelerationStructure(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* list,
		ID3D12Resource* bottomLevelResultAccelerationStructureBuffer,
		ID3D12Resource*& topLevelInstanceBuffer,
		ID3D12Resource*& resultAccelerationStructureBuffer,
		ID3D12Resource*& scratchAccelerationStructureBuffer);
	bool SetTopLevelTransform(ID3D12Resource* topLevelInstanceBuffer,
		ID3D12Resource* bottomLevelResultAccelerationStructureBuffer,
		ID3D12Resource* topLevelResultAccelerationStructureBuffer,
		ID3D12Resource* topLevelScratchAccelerationStructureBuffer,
		ID3D12GraphicsCommandList4* list,
		float rotation);
	bool CreateRayTracingStateObject(ID3D12Device5 * device,
		ID3D12StateObject * &stateObject,
		ID3D12RootSignature * &rayTracingRootSignature);
	bool CreateShaderRecordBuffers(ID3D12Device* device,
		ID3D12GraphicsCommandList* list, ID3D12StateObject* stateObject,
		ID3D12Resource* accelerationStructureBuffer,
		D3D12_GPU_DESCRIPTOR_HANDLE outputTextureUav,
		size_t& currentUploadOffset, ID3D12Resource* uploadBuffer,
		RayGenerationShaderRecord<32>& rayGenRecord,
		ShaderTable<32, 1>& missTable,
		ShaderTable<32, 1>& hitGroupTable);
	void CreateTexture2DUAV(ID3D12Device* device, ID3D12Resource* texture, ID3D12DescriptorHeap* heap,
		UINT descriptorSize, UINT descriptorOffset);
	void CreateTexture2DRTV(ID3D12Device* device, ID3D12Resource* texture, ID3D12DescriptorHeap* heap,
		UINT descriptorSize, UINT descriptorOffset);
	void DispatchRays(ID3D12GraphicsCommandList4* list,
		ID3D12StateObject* pipelineState,
		ID3D12DescriptorHeap* descriptorHeap,
		const RayGenerationShaderRecord<32>& rayGenRecord,
		const ShaderTable<32, 1>& missTable,
		const ShaderTable<32, 1>& hitGroupTable,
		unsigned int width, unsigned int height);

	template<size_t RootArgumentsByteSize>
	bool CreateShaderRecord(ID3D12StateObject* stateObject,
		ShaderRecord<RootArgumentsByteSize>& toSet,
		const wchar_t* shaderName, void* rootArgumentData,
		size_t dataSize = RootArgumentsByteSize);

private:
	ID3D12Resource* vertexBuffer2;
	//const int ROOT_ARGUMENTS_SIZE = 32;
	RayGenerationShaderRecord<32> rayGenRecord;
	ShaderTable<32, 1> missTable;
	ShaderTable<32, 1> hitGroupTable;
	ID3D12StateObject* stateObject = nullptr;
	ID3D12RootSignature* rayTracingRootSignature = nullptr;
	ID3D12Resource* writableTexture;
	ID3D12Resource* topLevelInstanceBuffer = nullptr;
	ID3D12Resource* topLevelResultAccelerationStructureBuffer = nullptr;
	ID3D12Resource* topLevelScratchAccelerationStructureBuffer = nullptr;
	ID3D12Resource* bottomLevelResultAccelerationStructureBuffer = nullptr;
	ID3D12Resource* bottomLevelScratchAccelerationStructureBuffer = nullptr;
	ID3D12GraphicsCommandList4* directCommandList4 = nullptr;
	ID3D12Device5* device5 = nullptr;
	ID3D12PipelineState* graphicsPSO;
	ID3D12RootSignature* rootSignature;
	std::vector<ID3D12Resource*> backbufferResources;
	IDXGISwapChain1* swapChain;
	ID3D12DescriptorHeap* descriptorHeapDSV;
	ID3D12DescriptorHeap* descriptorHeapRTV;
	ID3D12DescriptorHeap* descriptorHeapShaderBindable;
	UINT descriptorSizeShaderBindable, descriptorSizeRTV, descriptorSizeDSV;
	ID3D12Resource* uploadBuffer;
	ID3D12Heap* uploadHeap;
	ID3D12Resource* depthStencil;
	const unsigned int WINDOW_WIDTH = 1280;
	const unsigned int WINDOW_HEIGHT = 720;
	ID3D12Resource* texture;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	ID3D12Device* device;
	ID3D12CommandQueue* directCommandQueue;
	ID3D12CommandAllocator* directCommandAllocator;
	ID3D12GraphicsCommandList* directCommandList;
	ID3D12Fence* fence;
	size_t currentFenceValue = 0;
	ID3D12Heap* vertexHeap;
	ID3D12Resource* vertexBuffer;
	const unsigned int NR_OF_BACKBUFFERS = 2;
};

template<size_t RootArgumentsByteSize>
bool JRaytracing::CreateShaderRecord(ID3D12StateObject* stateObject,
	ShaderRecord<RootArgumentsByteSize>& toSet,
	const wchar_t* shaderName, void* rootArgumentData,
	size_t dataSize)
{
	ID3D12StateObjectProperties* stateObjectProperties = nullptr;
	if (FAILED(stateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProperties))))
		return false;

	memcpy(toSet.shaderIdentifier, stateObjectProperties->GetShaderIdentifier(shaderName),
		D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	memcpy(toSet.rootArguments, rootArgumentData, dataSize);

	stateObjectProperties->Release();
	return true;
}