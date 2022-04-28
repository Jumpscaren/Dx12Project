#include "JRaytracing.h"

bool JRaytracing::CheckSupportDXR(ID3D12Device* device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureData = {};

	HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureData,
		sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

	if (FAILED(hr))
		return false;

	return featureData.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
}

void JRaytracing::ThrowIfFailed(HRESULT hr, const std::exception& exception)
{
	if (FAILED(hr))
		throw exception;
}

void JRaytracing::EnableDebugLayer()
{
	ID3D12Debug* debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)),
		std::runtime_error("Could not get debug interface"));
	debugController->EnableDebugLayer();
	debugController->Release();
}

void JRaytracing::EnableGPUBasedValidation()
{
	ID3D12Debug1* debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)),
		std::runtime_error("Could not get debug interface"));
	debugController->SetEnableGPUBasedValidation(true);
	debugController->Release();
}

IDXGIFactory* JRaytracing::CreateFactory(UINT flags)
{
	IDXGIFactory* toReturn;

	HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create dxgi factory"));

	return toReturn;
}

IDXGIAdapter* JRaytracing::GetD3D12Adapter(IDXGIFactory* factory)
{
	unsigned int adapterIndex = 0;
	IDXGIAdapter* toReturn = nullptr;

	while (toReturn == nullptr)
	{
		HRESULT hr = factory->EnumAdapters(adapterIndex, &toReturn);
		ThrowIfFailed(hr, std::runtime_error("Could not find acceptable adapter"));
		ID3D12Device* temp;
		hr = D3D12CreateDevice(toReturn, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device),
			reinterpret_cast<void**>(&temp));
		if (FAILED(hr) || !CheckSupportDXR(temp))
		{
			toReturn->Release(); // Adapter did not support feature level 12.0, or did not support ray tracing
			toReturn = nullptr;
		}
		temp->Release();
		++adapterIndex;
	}

	return toReturn;
}

ID3D12CommandQueue* JRaytracing::CreateDirectCommandQueue(ID3D12Device* device)
{
	D3D12_COMMAND_QUEUE_DESC desc;
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = 0;
	desc.NodeMask = 0;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	ID3D12CommandQueue* toReturn;
	HRESULT hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create direct command queue"));

	return toReturn;
}

ID3D12CommandAllocator* JRaytracing::CreateDirectCommandAllocator(ID3D12Device* device)
{
	ID3D12CommandAllocator* toReturn;
	HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create direct command allocator"));

	return toReturn;
}

ID3D12GraphicsCommandList* JRaytracing::CreateDirectCommandList(ID3D12Device* device, ID3D12CommandAllocator* directAllocator)
{
	ID3D12GraphicsCommandList* toReturn;
	HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, directAllocator, nullptr, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create direct command list"));

	return toReturn;
}

ID3D12Fence* JRaytracing::CreateFence(ID3D12Device* device, UINT64 initialValue)
{
	ID3D12Fence* toReturn;
	HRESULT hr = device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create fence"));

	return toReturn;
}

ID3D12Heap* JRaytracing::CreateVertexHeap(ID3D12Device* device, size_t nrOfVertices)
{
	D3D12_HEAP_PROPERTIES properties;
	properties.Type = D3D12_HEAP_TYPE_DEFAULT;
	properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CreationNodeMask = 0;
	properties.VisibleNodeMask = 0;

	D3D12_HEAP_DESC desc;
	desc.SizeInBytes = sizeof(SimpleVertex) * nrOfVertices;
	desc.Properties = properties;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Flags = D3D12_HEAP_FLAG_NONE;

	ID3D12Heap* toReturn;

	HRESULT hr = device->CreateHeap(&desc, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create vertex heap"));

	return toReturn;
}

ID3D12Resource* JRaytracing::CreateVertexBuffer(ID3D12Device* device, ID3D12Heap* heapToAllocateIn,
	size_t heapOffset, size_t nrOfVertices)
{
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = sizeof(SimpleVertex) * nrOfVertices;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* toReturn;

	HRESULT hr = device->CreatePlacedResource(heapToAllocateIn, heapOffset, &desc,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create vertex buffer resource"));

	return toReturn;
}

ID3D12Heap* JRaytracing::CreateUploadHeap(ID3D12Device* device, size_t sizeInBytes)
{
	D3D12_HEAP_PROPERTIES properties;
	properties.Type = D3D12_HEAP_TYPE_UPLOAD;
	properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CreationNodeMask = 0;
	properties.VisibleNodeMask = 0;

	D3D12_HEAP_DESC desc;
	desc.SizeInBytes = sizeInBytes;
	desc.Properties = properties;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Flags = D3D12_HEAP_FLAG_NONE;

	ID3D12Heap* toReturn;

	HRESULT hr = device->CreateHeap(&desc, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create upload heap"));

	return toReturn;
}

ID3D12Resource* JRaytracing::CreateUploadBuffer(ID3D12Device* device, ID3D12Heap* heapToAllocateIn,
	size_t heapOffset, size_t sizeInBytes)
{
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = sizeInBytes;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* toReturn;

	HRESULT hr = device->CreatePlacedResource(heapToAllocateIn, heapOffset, &desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create upload buffer resource"));

	return toReturn;
}

void JRaytracing::TransitionResource(ID3D12Resource* resource, ID3D12GraphicsCommandList* commandList,
	D3D12_RESOURCE_STATES currentState, D3D12_RESOURCE_STATES newState)
{
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = currentState;
	barrier.Transition.StateAfter = newState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &barrier);
}

unsigned char* JRaytracing::CreateTextureData(unsigned int dimension, unsigned short mipLevels, unsigned int& totalByteSize)
{
	totalByteSize = dimension * dimension;
	unsigned int divider = 1;
	for (unsigned short mipLevel = 1; mipLevel < mipLevels; ++mipLevel)
	{
		divider *= 2;
		totalByteSize += dimension * dimension / divider;
	}
	totalByteSize *= 4;
	unsigned char* toReturn = new unsigned char[totalByteSize];
	unsigned int writtenBytes = 0;

	for (unsigned short mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
	{
		unsigned int currentDimension = dimension / pow(2, mipLevel);

		for (unsigned int h = 0; h < currentDimension; ++h)
		{
			for (unsigned int w = 0; w < currentDimension; ++w)
			{
				unsigned int basePos = writtenBytes + h * currentDimension * 4 + w * 4;
				toReturn[basePos + 0] = mipLevel % 3 == 0 ? 255 : 0;
				toReturn[basePos + 1] = mipLevel % 3 == 1 ? 255 : 0;
				toReturn[basePos + 2] = mipLevel % 3 == 2 ? 255 : 0;
				toReturn[basePos + 3] = 0;
			}
		}

		writtenBytes += 4 * currentDimension * currentDimension;
	}

	return toReturn;
}

ID3D12Resource* JRaytracing::CreateTexture2DResource(ID3D12Device* device, unsigned int width,
	unsigned int height, unsigned int mipLevels, DXGI_FORMAT format,
	D3D12_RESOURCE_FLAGS flags, D3D12_CLEAR_VALUE* clearColour)
{
	D3D12_HEAP_PROPERTIES heapProperties;
	ZeroMemory(&heapProperties, sizeof(heapProperties));
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = static_cast<UINT16>(mipLevels);
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;

	ID3D12Resource* toReturn;
	device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_COMMON, clearColour, IID_PPV_ARGS(&toReturn));

	return toReturn;
}

size_t JRaytracing::AlignAdress(size_t adress, size_t alignment)
{
	if ((0 == alignment) || (alignment & (alignment - 1)))
	{
		throw std::runtime_error("Error: non-pow2 alignment");
	}

	return ((adress + (alignment - 1)) & ~(alignment - 1));
}

size_t JRaytracing::MemcpyUploadData(unsigned char* mappedPtr, unsigned char* data, size_t uploadOffset, unsigned int alignment,
	UINT nrOfRows, UINT64 rowSizeInBytes, UINT rowPitch)
{
	unsigned int sourceOffset = 0;
	size_t destinationOffset = AlignAdress(uploadOffset, alignment);
	unsigned int writtenBytes = 0;
	for (UINT row = 0; row < nrOfRows; ++row)
	{
		std::memcpy(mappedPtr + destinationOffset, data + sourceOffset, rowSizeInBytes);
		sourceOffset += rowSizeInBytes;
		destinationOffset += rowPitch;
		writtenBytes += rowSizeInBytes;
	}

	return destinationOffset;
}

size_t JRaytracing::UploadData(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12Resource* uploadBuffer,
	ID3D12Resource* targetResource, unsigned char* data, size_t uploadOffset, unsigned int alignment,
	unsigned int subresource)
{
	D3D12_RANGE nothing = { 0, 0 };
	unsigned char* mappedPtr = nullptr;
	HRESULT hr = uploadBuffer->Map(0, &nothing, reinterpret_cast<void**>(&mappedPtr));
	ThrowIfFailed(hr, std::runtime_error("Could not map resource for uploading"));

	D3D12_RESOURCE_DESC resourceDesc = targetResource->GetDesc();
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT nrOfRows = 0;
	UINT64 rowSizeInBytes = 0;
	UINT64 totalBytes = 0;
	device->GetCopyableFootprints(&resourceDesc, subresource, 1, 0, &footprint, &nrOfRows, &rowSizeInBytes,
		&totalBytes);

	size_t newOffset = MemcpyUploadData(mappedPtr, data, uploadOffset, alignment, nrOfRows, rowSizeInBytes,
		footprint.Footprint.RowPitch);

	if (resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		D3D12_TEXTURE_COPY_LOCATION destination;
		destination.pResource = targetResource;
		destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		destination.SubresourceIndex = subresource;

		D3D12_TEXTURE_COPY_LOCATION source;
		source.pResource = uploadBuffer;
		source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		source.PlacedFootprint.Offset = AlignAdress(uploadOffset, alignment);
		source.PlacedFootprint.Footprint.Width = footprint.Footprint.Width;
		source.PlacedFootprint.Footprint.Height = footprint.Footprint.Height;
		source.PlacedFootprint.Footprint.Depth = footprint.Footprint.Depth;
		source.PlacedFootprint.Footprint.RowPitch = footprint.Footprint.RowPitch;
		source.PlacedFootprint.Footprint.Format = resourceDesc.Format;

		commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
	}
	else
	{
		commandList->CopyBufferRegion(targetResource, 0, uploadBuffer,
			AlignAdress(uploadOffset, alignment), resourceDesc.Width);
	}

	uploadBuffer->Unmap(0, nullptr);
	return newOffset;
}

void JRaytracing::ExecuteCommandList(ID3D12GraphicsCommandList* list, ID3D12CommandQueue* queue)
{
	HRESULT hr = list->Close();
	auto errorInfo = std::runtime_error("Could not close command list");
	ThrowIfFailed(hr, errorInfo);
	ID3D12CommandList* temp = list;
	queue->ExecuteCommandLists(1, &temp);
}

void JRaytracing::FlushCommandQueue(ID3D12Fence* fence, size_t& currentFenceValue, ID3D12CommandQueue* queue)
{
	++currentFenceValue;
	HRESULT hr = queue->Signal(fence, currentFenceValue);
	ThrowIfFailed(hr, std::runtime_error("Could not set fence signal"));

	if (fence->GetCompletedValue() < currentFenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, 0, 0, EVENT_ALL_ACCESS);
		hr = fence->SetEventOnCompletion(currentFenceValue, eventHandle);
		ThrowIfFailed(hr, std::runtime_error("Could not set event on fence completion"));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12DescriptorHeap* JRaytracing::CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
	UINT nrOfDescriptors, bool shaderVisible)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = type;
	desc.NumDescriptors = nrOfDescriptors;
	desc.Flags = (shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	desc.NodeMask = 0;

	ID3D12DescriptorHeap* toReturn;
	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create descriptor heap"));

	return toReturn;
}

void JRaytracing::CreateTexture2DSRV(ID3D12Device* device, ID3D12Resource* texture, ID3D12DescriptorHeap* heap,
	UINT descriptorSize, UINT descriptorOffset)
{
	D3D12_CPU_DESCRIPTOR_HANDLE heapHandle = heap->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += descriptorSize * descriptorOffset;
	device->CreateShaderResourceView(texture, nullptr, heapHandle);
}

void JRaytracing::CreateTexture2DDSV(ID3D12Device* device, ID3D12Resource* texture, ID3D12DescriptorHeap* heap,
	UINT descriptorSize, UINT descriptorOffset)
{
	D3D12_CPU_DESCRIPTOR_HANDLE heapHandle = heap->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += descriptorSize * descriptorOffset;
	device->CreateDepthStencilView(texture, nullptr, heapHandle);
}

IDXGISwapChain1* JRaytracing::CreateSwapChain(IDXGIFactory2* factory, ID3D12CommandQueue* queue, HWND window,
	unsigned int windowWidth, unsigned int windowHeight, unsigned int bufferCount)
{
	DXGI_SWAP_CHAIN_DESC1 desc;
	desc.Width = windowWidth;
	desc.Height = windowHeight;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Stereo = false;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = bufferCount;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.Flags = 0;

	IDXGISwapChain1* toReturn;
	HRESULT hr = factory->CreateSwapChainForHwnd(queue, window, &desc, nullptr, nullptr, &toReturn);
	ThrowIfFailed(hr, std::runtime_error("Could not create swap chain"));

	return toReturn;
}

void JRaytracing::CreateBackbufferRTVs(ID3D12Device* device, IDXGISwapChain* swapChain, unsigned int backbufferCount,
	ID3D12DescriptorHeap* descriptorHeap, UINT descriptorSize, UINT descriptorStartOffset,
	std::vector<ID3D12Resource*>& backbufferResources)
{
	D3D12_CPU_DESCRIPTOR_HANDLE heapHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += descriptorSize * descriptorStartOffset;

	for (unsigned int i = 0; i < backbufferCount; ++i)
	{
		ID3D12Resource* backbuffer = nullptr;
		HRESULT hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer));
		ThrowIfFailed(hr, std::runtime_error("Could not get backbuffer"));
		device->CreateRenderTargetView(backbuffer, nullptr, heapHandle);
		heapHandle.ptr += descriptorSize;
		backbufferResources.push_back(backbuffer);
	}
}

D3D12_ROOT_PARAMETER JRaytracing::CreateRootDescriptor(D3D12_ROOT_PARAMETER_TYPE typeOfView,
	D3D12_SHADER_VISIBILITY visibleShader, UINT shaderRegister, UINT registerSpace)
{
	D3D12_ROOT_PARAMETER toReturn;
	toReturn.ParameterType = typeOfView;
	toReturn.ShaderVisibility = visibleShader;
	toReturn.Descriptor.ShaderRegister = shaderRegister;
	toReturn.Descriptor.RegisterSpace = registerSpace;
	return toReturn;
}

D3D12_DESCRIPTOR_RANGE JRaytracing::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type,
	UINT nrOfDescriptors, UINT baseShaderRegister, UINT registerSpace)
{
	D3D12_DESCRIPTOR_RANGE toReturn;
	toReturn.RangeType = type;
	toReturn.NumDescriptors = nrOfDescriptors;
	toReturn.BaseShaderRegister = baseShaderRegister;
	toReturn.RegisterSpace = registerSpace;
	toReturn.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	return toReturn;
}

D3D12_ROOT_PARAMETER JRaytracing::CreateDescriptorTable(const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges,
	D3D12_SHADER_VISIBILITY visibleShader)
{
	// Do not use initializer list temporary vectors as input to this function
	// Memory needs to be valid after returning
	D3D12_ROOT_PARAMETER toReturn;
	toReturn.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	toReturn.ShaderVisibility = visibleShader;
	toReturn.DescriptorTable.NumDescriptorRanges = ranges.size();
	toReturn.DescriptorTable.pDescriptorRanges = ranges.size() != 0 ? ranges.data() : nullptr;
	return toReturn;
}

D3D12_STATIC_SAMPLER_DESC JRaytracing::CreateStaticSampler(D3D12_FILTER filter, UINT shaderRegister,
	D3D12_SHADER_VISIBILITY visibleShader, UINT registerSpace)
{
	D3D12_STATIC_SAMPLER_DESC toReturn;
	toReturn.Filter = filter;
	toReturn.AddressU = toReturn.AddressV = toReturn.AddressW =
		D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	toReturn.MipLODBias = 0.0f;
	toReturn.MaxAnisotropy = 16;
	toReturn.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	toReturn.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	toReturn.MinLOD = 0;
	toReturn.MaxLOD = D3D12_FLOAT32_MAX;
	toReturn.ShaderRegister = shaderRegister;
	toReturn.RegisterSpace = registerSpace;
	toReturn.ShaderVisibility = visibleShader;
	return toReturn;
}

ID3D12RootSignature* JRaytracing::CreateRootSignature(ID3D12Device* device,
	const std::vector<D3D12_ROOT_PARAMETER>& rootParameters,
	const std::vector<D3D12_STATIC_SAMPLER_DESC>& staticSamplers,
	D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	D3D12_ROOT_SIGNATURE_DESC desc;
	desc.NumParameters = static_cast<UINT>(rootParameters.size());
	desc.pParameters = rootParameters.size() == 0 ? nullptr : rootParameters.data();
	desc.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());;
	desc.pStaticSamplers = staticSamplers.size() == 0 ? nullptr : staticSamplers.data();;
	desc.Flags = flags;

	ID3DBlob* rootSignatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		&rootSignatureBlob, &errorBlob);

	if (FAILED(hr))
		throw std::runtime_error(static_cast<char*>(errorBlob->GetBufferPointer()));

	ID3D12RootSignature* toReturn;
	device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&toReturn));

	rootSignatureBlob->Release();

	return toReturn;
}

ID3DBlob* JRaytracing::LoadCSO(const std::string& filepath)
{
	std::ifstream file(filepath, std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("Could not open CSO file");

	file.seekg(0, std::ios_base::end);
	size_t size = static_cast<size_t>(file.tellg());
	file.seekg(0, std::ios_base::beg);

	ID3DBlob* toReturn = nullptr;
	HRESULT hr = D3DCreateBlob(size, &toReturn);

	if (FAILED(hr))
		throw std::runtime_error("Could not create blob when loading CSO");

	file.read(static_cast<char*>(toReturn->GetBufferPointer()), size);
	file.close();

	return toReturn;
}

D3D12_RASTERIZER_DESC JRaytracing::CreateRasterizerDesc()
{
	D3D12_RASTERIZER_DESC toReturn;
	toReturn.FillMode = D3D12_FILL_MODE_SOLID;
	toReturn.CullMode = D3D12_CULL_MODE_BACK;
	toReturn.FrontCounterClockwise = false;
	toReturn.DepthBias = 0;
	toReturn.DepthBiasClamp = 0.0f;
	toReturn.SlopeScaledDepthBias = 0.0f;
	toReturn.DepthClipEnable = true;
	toReturn.MultisampleEnable = false;
	toReturn.AntialiasedLineEnable = false;
	toReturn.ForcedSampleCount = 0;
	toReturn.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return toReturn;
}

D3D12_RENDER_TARGET_BLEND_DESC JRaytracing::CreateBlendDesc()
{
	D3D12_RENDER_TARGET_BLEND_DESC toReturn;
	toReturn.BlendEnable = false;
	toReturn.LogicOpEnable = false;
	toReturn.SrcBlend = D3D12_BLEND_ONE;
	toReturn.DestBlend = D3D12_BLEND_ZERO;
	toReturn.BlendOp = D3D12_BLEND_OP_ADD;
	toReturn.SrcBlendAlpha = D3D12_BLEND_ONE;
	toReturn.DestBlendAlpha = D3D12_BLEND_ZERO;
	toReturn.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	toReturn.LogicOp = D3D12_LOGIC_OP_NOOP;
	toReturn.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	return toReturn;
}

D3D12_DEPTH_STENCIL_DESC JRaytracing::CreateDepthStencilDesc()
{
	D3D12_DEPTH_STENCIL_DESC toReturn;
	toReturn.DepthEnable = true;
	toReturn.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	toReturn.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	toReturn.StencilEnable = false;
	toReturn.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	toReturn.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	toReturn.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	toReturn.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	toReturn.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	toReturn.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	toReturn.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	toReturn.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	toReturn.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	toReturn.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	return toReturn;
}

D3D12_STREAM_OUTPUT_DESC JRaytracing::CreateStreamOutputDesc()
{
	D3D12_STREAM_OUTPUT_DESC toReturn;
	toReturn.pSODeclaration = nullptr;
	toReturn.NumEntries = 0;
	toReturn.pBufferStrides = nullptr;
	toReturn.NumStrides = 0;
	toReturn.RasterizedStream = 0;
	return toReturn;
}

ID3D12PipelineState* JRaytracing::CreateGraphicsPipelineState(ID3D12Device* device,
	ID3D12RootSignature* rootSignature, const std::array<std::string, 5>& shaderPaths,
	DXGI_FORMAT dsvFormat, const std::vector<DXGI_FORMAT>& rtvFormats)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	desc.pRootSignature = rootSignature;
	std::array<D3D12_SHADER_BYTECODE*, 5> shaders = { &desc.VS, &desc.HS,
		&desc.DS, &desc.GS, &desc.PS };
	std::array<ID3DBlob*, 5> shaderBlobs;

	for (int i = 0; i < 5; ++i)
	{
		if (shaderPaths[i] != "")
		{
			shaderBlobs[i] = LoadCSO(shaderPaths[i]);
			(*shaders[i]).pShaderBytecode = shaderBlobs[i]->GetBufferPointer();
			(*shaders[i]).BytecodeLength = shaderBlobs[i]->GetBufferSize();
		}
	}

	desc.SampleMask = UINT_MAX;
	desc.RasterizerState = CreateRasterizerDesc();
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = static_cast<UINT>(rtvFormats.size());

	desc.BlendState.AlphaToCoverageEnable = false;
	desc.BlendState.IndependentBlendEnable = false;

	for (int i = 0; i < rtvFormats.size(); ++i)
	{
		desc.RTVFormats[i] = rtvFormats[i];
		desc.BlendState.RenderTarget[i] = CreateBlendDesc();
	}

	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.DSVFormat = dsvFormat;
	desc.DepthStencilState = CreateDepthStencilDesc();
	desc.StreamOutput = CreateStreamOutputDesc();
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hr;
	ID3D12PipelineState* toReturn;
	hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&toReturn));
	ThrowIfFailed(hr, std::runtime_error("Could not create graphics pipeline state"));

	return toReturn;
}

void JRaytracing::ResetCommandMemory(ID3D12CommandAllocator* allocator, ID3D12GraphicsCommandList* commandList)
{
	HRESULT hr = allocator->Reset();
	ThrowIfFailed(hr, std::runtime_error("Could not reset allocator"));
	hr = commandList->Reset(allocator, nullptr); // nullptr is initial state, no initial state
	ThrowIfFailed(hr, std::runtime_error("Could not reset command list"));
}

void JRaytracing::PreDraw(ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* shaderBindHeap,
	ID3D12RootSignature* rootSignature, ID3D12PipelineState* pipelineState,
	ID3D12Resource* backbuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, unsigned int width, unsigned int height)
{
	// Set descriptor heaps before setting root signature if direct heap access is needed
	commandList->SetDescriptorHeaps(1, &shaderBindHeap);
	commandList->SetGraphicsRootSignature(rootSignature);
	commandList->SetPipelineState(pipelineState);

	D3D12_RESOURCE_BARRIER backbufferTransitionBarrier;
	backbufferTransitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	backbufferTransitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	backbufferTransitionBarrier.Transition.pResource = backbuffer;
	backbufferTransitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	backbufferTransitionBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT; // Same as common
	backbufferTransitionBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &backbufferTransitionBarrier);

	float clearColour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColour, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
	D3D12_VIEWPORT viewport = { 0, 0, width, height, 0.0f, 1.0f };
	commandList->RSSetViewports(1, &viewport);
	D3D12_RECT scissorRect = { 0, 0, width, height };
	commandList->RSSetScissorRects(1, &scissorRect);
}

void JRaytracing::Draw(ID3D12GraphicsCommandList* commandList, ID3D12Resource* vertexBuffer,
	ID3D12DescriptorHeap* shaderBindableHeap, unsigned int vertexCount)
{
	commandList->SetGraphicsRootShaderResourceView(0, vertexBuffer->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(1, shaderBindableHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->DrawInstanced(vertexCount, 1, 0, 0);
}

void JRaytracing::FinishFrame(ID3D12GraphicsCommandList* commandList, ID3D12Resource* backbuffer)
{
	D3D12_RESOURCE_BARRIER backbufferTransitionBarrier;
	backbufferTransitionBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	backbufferTransitionBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	backbufferTransitionBarrier.Transition.pResource = backbuffer;
	backbufferTransitionBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	backbufferTransitionBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	backbufferTransitionBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList->ResourceBarrier(1, &backbufferTransitionBarrier);
}


//-----------------------------Functions new for ray tracing here-----------------------------
// Check support function at the top is also new

bool JRaytracing::CreateCommitedBuffer(ID3D12Device* device, ID3D12Resource*& toSet,
	D3D12_HEAP_TYPE heapType, size_t bufferSize, D3D12_RESOURCE_FLAGS flags,
	D3D12_RESOURCE_STATES initialState)
{
	D3D12_HEAP_PROPERTIES heapProperties;
	ZeroMemory(&heapProperties, sizeof(heapProperties));
	heapProperties.Type = heapType;

	D3D12_RESOURCE_DESC bufferDesc;
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	bufferDesc.Width = bufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = flags;

	HRESULT hr = device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferDesc, initialState, nullptr, IID_PPV_ARGS(&toSet));

	return hr == S_OK;
}

bool JRaytracing::BuildAccelerationStructureHelper(ID3D12Device5* device,
	ID3D12GraphicsCommandList4* list,
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs,
	UINT64 resultBufferSize, ID3D12Resource*& resultBuffer,
	UINT64 scratchBufferSize, ID3D12Resource*& scratchBuffer)
{
	if (!CreateCommitedBuffer(device, resultBuffer, D3D12_HEAP_TYPE_DEFAULT,
		resultBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE))
	{
		return false;
	}

	if (!CreateCommitedBuffer(device, scratchBuffer, D3D12_HEAP_TYPE_DEFAULT,
		scratchBufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS))
	{
		resultBuffer->Release();
		return false;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationStructureDesc;
	accelerationStructureDesc.DestAccelerationStructureData =
		resultBuffer->GetGPUVirtualAddress();
	accelerationStructureDesc.Inputs = inputs;
	accelerationStructureDesc.SourceAccelerationStructureData = NULL;
	accelerationStructureDesc.ScratchAccelerationStructureData =
		scratchBuffer->GetGPUVirtualAddress();

	list->BuildRaytracingAccelerationStructure(&accelerationStructureDesc, 0, nullptr);

	return true;
}

bool JRaytracing::BuildBottomLevelAccelerationStructure(ID3D12Device5* device,
	ID3D12GraphicsCommandList4* list, D3D12_GPU_VIRTUAL_ADDRESS vertexBufferFirstPosition,
	ID3D12Resource*& resultAccelerationStructureBuffer,
	ID3D12Resource*& scratchAccelerationStructureBuffer)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	bottomLevelInputs.NumDescs = 1;
	bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDescriptions[1];
	geometryDescriptions[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDescriptions[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	geometryDescriptions[0].Triangles.Transform3x4 = NULL;
	geometryDescriptions[0].Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
	geometryDescriptions[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDescriptions[0].Triangles.IndexCount = 0;
	geometryDescriptions[0].Triangles.VertexCount = 3;
	geometryDescriptions[0].Triangles.IndexBuffer = NULL;
	geometryDescriptions[0].Triangles.VertexBuffer.StartAddress = vertexBufferFirstPosition;
	geometryDescriptions[0].Triangles.VertexBuffer.StrideInBytes = sizeof(SimpleVertex);
	bottomLevelInputs.pGeometryDescs = geometryDescriptions;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &prebuildInfo);

	return BuildAccelerationStructureHelper(device, list, bottomLevelInputs,
		prebuildInfo.ResultDataMaxSizeInBytes, resultAccelerationStructureBuffer,
		prebuildInfo.ScratchDataSizeInBytes, scratchAccelerationStructureBuffer);
}

bool JRaytracing::BuildTopLevelAccelerationStructure(ID3D12Device5* device,
	ID3D12GraphicsCommandList4* list,
	ID3D12Resource* bottomLevelResultAccelerationStructureBuffer,
	ID3D12Resource*& topLevelInstanceBuffer,
	ID3D12Resource*& resultAccelerationStructureBuffer,
	ID3D12Resource*& scratchAccelerationStructureBuffer)
{
	if (!CreateCommitedBuffer(device, topLevelInstanceBuffer, D3D12_HEAP_TYPE_UPLOAD,
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC), D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ))
	{
		return false;
	}

	D3D12_RAYTRACING_INSTANCE_DESC instancingDesc;
	ZeroMemory(&instancingDesc.Transform, sizeof(float) * 12);
	instancingDesc.Transform[0][0] = instancingDesc.Transform[1][1] =
		instancingDesc.Transform[2][2] = 1;
	instancingDesc.InstanceID = 0;
	instancingDesc.InstanceMask = 0xFF;
	instancingDesc.InstanceContributionToHitGroupIndex = 0;
	instancingDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instancingDesc.AccelerationStructure =
		bottomLevelResultAccelerationStructureBuffer->GetGPUVirtualAddress();

	D3D12_RANGE nothing = { 0, 0 };
	unsigned char* mappedPtr = nullptr;
	HRESULT hr = topLevelInstanceBuffer->Map(0, &nothing, reinterpret_cast<void**>(&mappedPtr));

	if (FAILED(hr))
		return false;

	memcpy(mappedPtr, &instancingDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
	topLevelInstanceBuffer->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.InstanceDescs = topLevelInstanceBuffer->GetGPUVirtualAddress();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &prebuildInfo);

	return BuildAccelerationStructureHelper(device, list, topLevelInputs,
		prebuildInfo.ResultDataMaxSizeInBytes, resultAccelerationStructureBuffer,
		prebuildInfo.ScratchDataSizeInBytes, scratchAccelerationStructureBuffer);
}

bool JRaytracing::SetTopLevelTransform(ID3D12Resource* topLevelInstanceBuffer,
	ID3D12Resource* bottomLevelResultAccelerationStructureBuffer,
	ID3D12Resource* topLevelResultAccelerationStructureBuffer,
	ID3D12Resource* topLevelScratchAccelerationStructureBuffer,
	ID3D12GraphicsCommandList4* list,
	float rotation)
{
	DirectX::XMFLOAT3X4 toStore;
	DirectX::XMStoreFloat3x4(&toStore, DirectX::XMMatrixRotationY(rotation));

	D3D12_RAYTRACING_INSTANCE_DESC instancingDesc;
	memcpy(instancingDesc.Transform, &toStore, sizeof(instancingDesc.Transform));
	instancingDesc.InstanceID = 0;
	instancingDesc.InstanceMask = 0xFF;
	instancingDesc.InstanceContributionToHitGroupIndex = 0;
	instancingDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instancingDesc.AccelerationStructure =
		bottomLevelResultAccelerationStructureBuffer->GetGPUVirtualAddress();

	D3D12_RANGE nothing = { 0, 0 };
	unsigned char* mappedPtr = nullptr;
	HRESULT hr = topLevelInstanceBuffer->Map(0, &nothing, reinterpret_cast<void**>(&mappedPtr));

	if (FAILED(hr))
		return false;

	memcpy(mappedPtr, &instancingDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
	topLevelInstanceBuffer->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.InstanceDescs = topLevelInstanceBuffer->GetGPUVirtualAddress();

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelerationStructureDesc;
	accelerationStructureDesc.DestAccelerationStructureData =
		topLevelResultAccelerationStructureBuffer->GetGPUVirtualAddress();
	accelerationStructureDesc.Inputs = topLevelInputs;
	accelerationStructureDesc.SourceAccelerationStructureData = NULL;
	accelerationStructureDesc.ScratchAccelerationStructureData =
		topLevelScratchAccelerationStructureBuffer->GetGPUVirtualAddress();

	list->BuildRaytracingAccelerationStructure(&accelerationStructureDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = topLevelResultAccelerationStructureBuffer;
	list->ResourceBarrier(1, &uavBarrier);

	return true;
}

bool JRaytracing::CreateRayTracingStateObject(ID3D12Device5* device,
	ID3D12StateObject*& stateObject,
	ID3D12RootSignature*& rayTracingRootSignature)
{
	D3D12_STATE_SUBOBJECT stateSubobjects[6];

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
	shaderConfig.MaxPayloadSizeInBytes = sizeof(float) * 3;
	shaderConfig.MaxAttributeSizeInBytes = sizeof(float) * 2;
	stateSubobjects[0].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	stateSubobjects[0].pDesc = &shaderConfig;

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = 1;
	stateSubobjects[1].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	stateSubobjects[1].pDesc = &pipelineConfig;

	D3D12_LOCAL_ROOT_SIGNATURE localRootSignature;
	std::vector<D3D12_ROOT_PARAMETER> rootParameters;
	rootParameters.push_back(CreateRootDescriptor(D3D12_ROOT_PARAMETER_TYPE_SRV, D3D12_SHADER_VISIBILITY_ALL, 0));
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = { CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0) };
	rootParameters.push_back(CreateDescriptorTable(ranges, D3D12_SHADER_VISIBILITY_ALL));
	rayTracingRootSignature = CreateRootSignature(device, rootParameters, {},
		D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
	localRootSignature.pLocalRootSignature = rayTracingRootSignature;
	stateSubobjects[2].Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	stateSubobjects[2].pDesc = &localRootSignature;

	D3D12_DXIL_LIBRARY_DESC dxilLibrary;
	ID3DBlob* libraryBlob = LoadCSO("x64/Debug/RayTracingShaders.cso");
	dxilLibrary.DXILLibrary.pShaderBytecode = libraryBlob->GetBufferPointer();
	dxilLibrary.DXILLibrary.BytecodeLength = libraryBlob->GetBufferSize();
	dxilLibrary.NumExports = 0;
	dxilLibrary.pExports = nullptr;
	stateSubobjects[3].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	stateSubobjects[3].pDesc = &dxilLibrary;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exportsAssociation;
	exportsAssociation.pSubobjectToAssociate = &stateSubobjects[2]; //LOCAL ROOT SIGNATURE
	exportsAssociation.NumExports = 0;
	exportsAssociation.pExports = nullptr;
	stateSubobjects[4].Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	stateSubobjects[4].pDesc = &exportsAssociation;

	D3D12_HIT_GROUP_DESC hitGroupDesc;
	hitGroupDesc.HitGroupExport = L"HitGroup";
	hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupDesc.AnyHitShaderImport = nullptr;
	hitGroupDesc.ClosestHitShaderImport = L"ClosestHitShader";
	hitGroupDesc.IntersectionShaderImport = nullptr;
	stateSubobjects[5].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	stateSubobjects[5].pDesc = &hitGroupDesc;

	D3D12_STATE_OBJECT_DESC description;
	description.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	description.NumSubobjects = ARRAYSIZE(stateSubobjects);
	description.pSubobjects = stateSubobjects;

	HRESULT hr = device->CreateStateObject(&description, IID_PPV_ARGS(&stateObject));

	return hr == S_OK;
}

bool JRaytracing::CreateShaderRecordBuffers(ID3D12Device* device,
	ID3D12GraphicsCommandList* list, ID3D12StateObject* stateObject,
	ID3D12Resource* accelerationStructureBuffer,
	D3D12_GPU_DESCRIPTOR_HANDLE outputTextureUav,
	size_t& currentUploadOffset, ID3D12Resource* uploadBuffer,
	RayGenerationShaderRecord<32>& rayGenRecord,
	ShaderTable<32, 1>& missTable,
	ShaderTable<32, 1>& hitGroupTable)
{
	const int ROOT_ARGUMENT_SIZE = 32;
	unsigned char rootArgumentData[ROOT_ARGUMENT_SIZE];
	auto rootArg1 = accelerationStructureBuffer->GetGPUVirtualAddress();
	auto rootArg2 = outputTextureUav;
	memcpy(rootArgumentData, &rootArg1, sizeof(rootArg1));
	memcpy(rootArgumentData + sizeof(rootArg1), &rootArg2, sizeof(rootArg2));


	ShaderRecord<ROOT_ARGUMENT_SIZE> rayGenRecordData;
	if (!CreateShaderRecord(stateObject, rayGenRecordData, L"RayGenerationShader", rootArgumentData))
		return false;

	if (!CreateCommitedBuffer(device, rayGenRecord.buffer, D3D12_HEAP_TYPE_DEFAULT, sizeof(rayGenRecordData),
		D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST))
	{
		return false;
	}

	currentUploadOffset = UploadData(device, list, uploadBuffer, rayGenRecord.buffer,
		reinterpret_cast<unsigned char*>(&rayGenRecordData), currentUploadOffset,
		alignof(ShaderRecord<ROOT_ARGUMENT_SIZE>));

	TransitionResource(rayGenRecord.buffer, list, D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);



	ShaderRecord<ROOT_ARGUMENT_SIZE> missRecordData;
	if (!CreateShaderRecord(stateObject, missRecordData, L"MissShader", rootArgumentData))
		return false;

	if (!CreateCommitedBuffer(device, missTable.buffer, D3D12_HEAP_TYPE_DEFAULT, sizeof(missRecordData),
		D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST))
	{
		return false;
	}

	currentUploadOffset = UploadData(device, list, uploadBuffer, missTable.buffer,
		reinterpret_cast<unsigned char*>(&missRecordData), currentUploadOffset,
		alignof(ShaderRecord<ROOT_ARGUMENT_SIZE>));

	TransitionResource(missTable.buffer, list, D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);



	ShaderRecord<ROOT_ARGUMENT_SIZE> hitGroupRecordData;
	if (!CreateShaderRecord(stateObject, hitGroupRecordData, L"HitGroup", rootArgumentData))
		return false;

	if (!CreateCommitedBuffer(device, hitGroupTable.buffer, D3D12_HEAP_TYPE_DEFAULT, sizeof(hitGroupRecordData),
		D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST))
	{
		return false;
	}

	currentUploadOffset = UploadData(device, list, uploadBuffer, hitGroupTable.buffer,
		reinterpret_cast<unsigned char*>(&hitGroupRecordData), currentUploadOffset,
		alignof(ShaderRecord<ROOT_ARGUMENT_SIZE>));

	TransitionResource(hitGroupTable.buffer, list, D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	return true;
}

void JRaytracing::CreateTexture2DUAV(ID3D12Device* device, ID3D12Resource* texture, ID3D12DescriptorHeap* heap,
	UINT descriptorSize, UINT descriptorOffset)
{
	D3D12_CPU_DESCRIPTOR_HANDLE heapHandle = heap->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += descriptorSize * descriptorOffset;
	device->CreateUnorderedAccessView(texture, nullptr, nullptr, heapHandle);
}

void JRaytracing::CreateTexture2DRTV(ID3D12Device* device, ID3D12Resource* texture, ID3D12DescriptorHeap* heap,
	UINT descriptorSize, UINT descriptorOffset)
{
	D3D12_CPU_DESCRIPTOR_HANDLE heapHandle = heap->GetCPUDescriptorHandleForHeapStart();
	heapHandle.ptr += descriptorSize * descriptorOffset;
	device->CreateRenderTargetView(texture, nullptr, heapHandle);
}

void JRaytracing::DispatchRays(ID3D12GraphicsCommandList4* list,
	ID3D12StateObject* pipelineState,
	ID3D12DescriptorHeap* descriptorHeap,
	const RayGenerationShaderRecord<32>& rayGenRecord,
	const ShaderTable<32, 1>& missTable,
	const ShaderTable<32, 1>& hitGroupTable,
	unsigned int width, unsigned int height)
{
	D3D12_DISPATCH_RAYS_DESC desc;
	desc.RayGenerationShaderRecord = rayGenRecord.GetGpuAdressRange();
	desc.MissShaderTable = missTable.GetGpuAdressRangeAndStride();
	desc.HitGroupTable = hitGroupTable.GetGpuAdressRangeAndStride();
	desc.CallableShaderTable.StartAddress = 0;
	desc.CallableShaderTable.SizeInBytes = 0;
	desc.CallableShaderTable.StrideInBytes = 0;
	desc.Width = width;
	desc.Height = height;
	desc.Depth = 1;

	list->SetDescriptorHeaps(1, &descriptorHeap);
	list->SetPipelineState1(pipelineState);
	list->DispatchRays(&desc);
}

void JRaytracing::setup(HWND hwnd)
{
	EnableDebugLayer();
	EnableGPUBasedValidation();

	factory = CreateFactory();
	adapter = GetD3D12Adapter(factory);
	D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
	directCommandQueue = CreateDirectCommandQueue(device);
	directCommandAllocator = CreateDirectCommandAllocator(device);
	directCommandList = CreateDirectCommandList(device, directCommandAllocator);
	fence = CreateFence(device);
	vertexHeap = CreateVertexHeap(device, 1024 * 64 * 2);
	vertexBuffer = CreateVertexBuffer(device, vertexHeap, 0, 3);

	SimpleVertex triangle[3] =
	{
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f}},
		{{0.0f, 0.5f, 0.0f}, {0.5f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {1.0f, 1.0f}}
	};

	unsigned int TEXTURE_DIMENSION = 1024;
	unsigned int TEXTURE_MIPS = 10;
	texture = CreateTexture2DResource(device, TEXTURE_DIMENSION, TEXTURE_DIMENSION, TEXTURE_MIPS);
	unsigned int textureDataSize = 0;
	unsigned char* textureData = CreateTextureData(TEXTURE_DIMENSION, TEXTURE_MIPS, textureDataSize);

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthStencil = CreateTexture2DResource(device, WINDOW_WIDTH, WINDOW_HEIGHT, 1, DXGI_FORMAT_D32_FLOAT,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, &depthClearValue);

	uploadHeap = CreateUploadHeap(device, 10000000);
	uploadBuffer = CreateUploadBuffer(device, uploadHeap, 0, 10000000);
	size_t currentUploadOffset = 0;
	size_t currentTextureOffset = 0;
	for (unsigned int mipLevel = 0; mipLevel < TEXTURE_MIPS; ++mipLevel)
	{
		currentUploadOffset = UploadData(device, directCommandList, uploadBuffer, texture,
			textureData + currentTextureOffset, currentUploadOffset,
			D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, mipLevel);
		currentTextureOffset += (TEXTURE_DIMENSION * TEXTURE_DIMENSION * 4) / pow(4, mipLevel);
	}
	//Texture needs transition. Promoted common -> copy_dest but does not decay back
	TransitionResource(texture, directCommandList, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	delete[] textureData;

	// Cannot promote to depth_write, need to be done manually
	TransitionResource(depthStencil, directCommandList, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	currentUploadOffset = UploadData(device, directCommandList, uploadBuffer, vertexBuffer,
		reinterpret_cast<unsigned char*>(triangle), currentUploadOffset, alignof(SimpleVertex));
	//Buffer does not need transition. Promoted common -> copy_dest then decays copy_dest -> common
	//Same method applies later during drawing but common -> non_pixel_shader_resource and then back to common when done

	ExecuteCommandList(directCommandList, directCommandQueue);
	FlushCommandQueue(fence, currentFenceValue, directCommandQueue);
	currentUploadOffset = 0;

	descriptorSizeShaderBindable = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	descriptorHeapShaderBindable = CreateDescriptorHeap(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, true);
	descriptorHeapRTV = CreateDescriptorHeap(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NR_OF_BACKBUFFERS + 1, false);
	descriptorHeapDSV = CreateDescriptorHeap(device,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	CreateTexture2DSRV(device, texture, descriptorHeapShaderBindable, descriptorSizeShaderBindable, 0);
	CreateTexture2DDSV(device, depthStencil, descriptorHeapDSV, descriptorSizeDSV, 0);

	HWND window = hwnd;//InitializeWindow(hInstance, WINDOW_WIDTH, WINDOW_HEIGHT);
	IDXGIFactory2* tempFactory;
	factory->QueryInterface(IID_PPV_ARGS(&tempFactory));
	swapChain = CreateSwapChain(tempFactory, directCommandQueue, window, WINDOW_WIDTH, WINDOW_HEIGHT, NR_OF_BACKBUFFERS);

	CreateBackbufferRTVs(device, swapChain, NR_OF_BACKBUFFERS, descriptorHeapRTV, descriptorSizeRTV, 0, backbufferResources);

	std::vector<D3D12_ROOT_PARAMETER> rootParameters;
	rootParameters.push_back(CreateRootDescriptor(D3D12_ROOT_PARAMETER_TYPE_SRV, D3D12_SHADER_VISIBILITY_VERTEX, 0));
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = { CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0) };
	rootParameters.push_back(CreateDescriptorTable(ranges, D3D12_SHADER_VISIBILITY_PIXEL));
	rootParameters.push_back(CreateRootDescriptor(D3D12_ROOT_PARAMETER_TYPE_SRV, D3D12_SHADER_VISIBILITY_PIXEL, 1));
	rootSignature = CreateRootSignature(device, rootParameters,
		{ CreateStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, 0) });
	graphicsPSO = CreateGraphicsPipelineState(device, rootSignature,
		{ "x64/Debug/VertexShader1.cso", "", "", "", "x64/Debug/PixelShader1.cso" }, DXGI_FORMAT_D32_FLOAT, { DXGI_FORMAT_R8G8B8A8_UNORM });

	// -------------------- Below are the major new additions to the demo for ray tracing -------------------

	if (!CheckSupportDXR(device))
		assert(false);

	HRESULT hr = device->QueryInterface(__uuidof(ID3D12Device5),
		reinterpret_cast<void**>(&device5));

	if (FAILED(hr))
		assert(false);

	hr = directCommandList->QueryInterface(__uuidof(ID3D12GraphicsCommandList4),
		reinterpret_cast<void**>(&directCommandList4));

	if (FAILED(hr))
		assert(false);

	ResetCommandMemory(directCommandAllocator, directCommandList4);

	TransitionResource(vertexBuffer, directCommandList, D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	if (!BuildBottomLevelAccelerationStructure(device5, directCommandList4,
		vertexBuffer->GetGPUVirtualAddress(),
		bottomLevelResultAccelerationStructureBuffer,
		bottomLevelScratchAccelerationStructureBuffer))
	{
		assert(false);
	}

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = bottomLevelResultAccelerationStructureBuffer;
	directCommandList4->ResourceBarrier(1, &uavBarrier);

	if (!BuildTopLevelAccelerationStructure(device5, directCommandList4,
		bottomLevelResultAccelerationStructureBuffer,
		topLevelInstanceBuffer, topLevelResultAccelerationStructureBuffer,
		topLevelScratchAccelerationStructureBuffer))
	{
		assert(false);
	}

	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = topLevelResultAccelerationStructureBuffer;
	directCommandList4->ResourceBarrier(1, &uavBarrier);

	D3D12_CLEAR_VALUE writableClearValue;
	writableClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	writableClearValue.Color[0] = writableClearValue.Color[1] =
		writableClearValue.Color[2] = writableClearValue.Color[3] = 0.0f;
	writableTexture = CreateTexture2DResource(device,
		WINDOW_WIDTH, WINDOW_HEIGHT, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS |
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &writableClearValue);
	CreateTexture2DUAV(device, writableTexture, descriptorHeapShaderBindable,
		descriptorSizeShaderBindable, 1);
	CreateTexture2DRTV(device, writableTexture, descriptorHeapRTV,
		descriptorSizeRTV, NR_OF_BACKBUFFERS);

	if (!CreateRayTracingStateObject(device5, stateObject, rayTracingRootSignature))
		assert(false);

	auto handleOutputUAV = descriptorHeapShaderBindable->GetGPUDescriptorHandleForHeapStart();
	handleOutputUAV.ptr += descriptorSizeShaderBindable; // Offset to descriptor 1 which is our UAV
	if (!CreateShaderRecordBuffers(device, directCommandList, stateObject,
		topLevelResultAccelerationStructureBuffer, handleOutputUAV,
		currentUploadOffset, uploadBuffer, rayGenRecord, missTable, hitGroupTable))
	{
		assert(false);
	}

	SimpleVertex quad[6] =
	{
		{{-0.75f, -0.75f, 0.5f}, {0.0f, 1.0f}},
		{{-0.75f, 0.75f, 0.5f}, {0.0f, 0.0f}},
		{{0.75f, 0.75f, 0.5f}, {1.0f, 0.0f}},

		{{-0.75f, -0.75f, 0.5f}, {0.0f, 1.0f}},
		{{0.75f, 0.75f, 0.5f}, {1.0f, 0.0f}},
		{{0.75f, -0.75f, 0.5f}, {1.0f, 1.0f}},
	};
	vertexBuffer2 = CreateVertexBuffer(device, vertexHeap, 1024 * 64, 6);
	currentUploadOffset = UploadData(device, directCommandList, uploadBuffer, vertexBuffer2,
		reinterpret_cast<unsigned char*>(quad), currentUploadOffset, alignof(SimpleVertex));

	ExecuteCommandList(directCommandList4, directCommandQueue);
	FlushCommandQueue(fence, currentFenceValue, directCommandQueue);
}

void JRaytracing::run()
{
	static unsigned int currentBackbuffer = NR_OF_BACKBUFFERS - 1;
	static float rotationValue = 0.0f;

	ResetCommandMemory(directCommandAllocator, directCommandList);

	if (!SetTopLevelTransform(topLevelInstanceBuffer, bottomLevelResultAccelerationStructureBuffer,
		topLevelResultAccelerationStructureBuffer, topLevelScratchAccelerationStructureBuffer,
		directCommandList4, rotationValue))
	{
		assert(false);
	}

	rotationValue += 0.005f;

	currentBackbuffer = (currentBackbuffer + 1) % NR_OF_BACKBUFFERS;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = descriptorHeapRTV->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = descriptorHeapDSV->GetCPUDescriptorHandleForHeapStart();

	rtvHandle.ptr += descriptorSizeRTV * NR_OF_BACKBUFFERS; // Our new texture, not the backbuffer
	PreDraw(directCommandList, descriptorHeapShaderBindable, rootSignature, graphicsPSO,
		writableTexture, rtvHandle, dsvHandle, WINDOW_WIDTH, WINDOW_HEIGHT);

	directCommandList->SetGraphicsRootShaderResourceView(2,
		topLevelResultAccelerationStructureBuffer->GetGPUVirtualAddress());
	Draw(directCommandList, vertexBuffer2, descriptorHeapShaderBindable, 6);

	TransitionResource(writableTexture, directCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	DispatchRays(directCommandList4, stateObject, descriptorHeapShaderBindable,
		rayGenRecord, missTable, hitGroupTable, WINDOW_WIDTH, WINDOW_HEIGHT);

	TransitionResource(writableTexture, directCommandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	TransitionResource(backbufferResources[currentBackbuffer], directCommandList,
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	directCommandList->CopyResource(backbufferResources[currentBackbuffer], writableTexture);

	TransitionResource(backbufferResources[currentBackbuffer], directCommandList,
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	TransitionResource(writableTexture, directCommandList, D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_PRESENT); // Not optimal as we could transition to render target
	// And skip the initial transition, but done here for simplicity and clarity

	ExecuteCommandList(directCommandList, directCommandQueue);
	swapChain->Present(0, 0);
	FlushCommandQueue(fence, currentFenceValue, directCommandQueue);
}