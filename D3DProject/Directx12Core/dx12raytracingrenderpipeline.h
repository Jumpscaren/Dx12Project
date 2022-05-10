#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include "dx12buffermanager.h"

template<size_t root_arguments_byte_size>
struct ShaderRecordF
{
	unsigned char shader_identifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES]; //Not a string, but data for a 'pointer' to the shader
	unsigned char root_arguments[root_arguments_byte_size];
};

struct RayShaderRecord
{
	UINT root_argument_size;
	UINT shader_record_size;
	unsigned char* shader_record_data;

	~RayShaderRecord()
	{
		//Fix this later
		//delete[] shader_record_data;
	}
};

struct RayShaderRecordTable
{
	UINT root_argument_size;
	UINT entire_shader_record_size;
	unsigned char* multiple_shader_record_data;
	BufferResource buffer = {};

	void CopyShaderRecordData(std::vector<RayShaderRecord> ray_shader_records)
	{
		UINT same_size_shader_record;

		root_argument_size = ray_shader_records[0].root_argument_size;

		UINT entire_size = 0;
		for (int i = 0; i < ray_shader_records.size(); ++i)
		{
			entire_size += ray_shader_records[i].shader_record_size;
		}
		
		multiple_shader_record_data = new unsigned char[entire_size];

		UINT offset = 0;
		for (int i = 0; i < ray_shader_records.size(); ++i)
		{
			//unsigned char* ptr = multiple_shader_record_data + ray_shader_records[i].shader_record_size;
			memcpy(multiple_shader_record_data + offset, ray_shader_records[i].shader_record_data, ray_shader_records[i].shader_record_size);
			offset += ray_shader_records[i].shader_record_size;
			//entire_size += ray_shader_records[i].shader_record_size;
		}

		entire_shader_record_size = entire_size;
	}

	~RayShaderRecordTable()
	{
		//delete[] multiple_shader_record_data;
	}

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetGpuAdressRangeAndStride() const
	{
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE return_address;
		return_address.StartAddress = buffer.buffer->GetGPUVirtualAddress();
		return_address.SizeInBytes = entire_shader_record_size;
		return_address.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			+ root_argument_size;

		return return_address;
	}
};

template<size_t root_arguments_byte_size>
struct RayGenerationShaderRecordF
{
	BufferResource buffer = {};

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE GetGpuAdressRange() const
	{
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE return_address;
		return_address.StartAddress = buffer.buffer->GetGPUVirtualAddress();
		return_address.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			+ root_arguments_byte_size;

		return return_address;
	}

	~RayGenerationShaderRecordF()
	{

	};
};

template<size_t root_arguments_byte_size, short nr_of_records>
struct ShaderTableF
{
	BufferResource buffer = {};

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE GetGpuAdressRangeAndStride() const
	{
		D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE return_address;
		return_address.StartAddress = buffer.buffer->GetGPUVirtualAddress();
		return_address.SizeInBytes = (D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			+ root_arguments_byte_size) * nr_of_records;
		return_address.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			+ root_arguments_byte_size;

		return return_address;
	}

	~ShaderTableF()
	{

	};
};

class dx12raytracingrenderpipeline
{
private:
	Microsoft::WRL::ComPtr<ID3D12StateObject> m_raytracing_state_object;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracing_local_root_signature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracing_global_root_signature;

	RayGenerationShaderRecordF<32> m_ray_gen_record;
	ShaderTableF<32, 1> m_miss_record;
	ShaderTableF<32, 2> m_hit_record;

	std::vector<D3D12_ROOT_PARAMETER> m_local_root_parameters;
	std::vector<D3D12_ROOT_PARAMETER> m_global_root_parameters;
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> m_ranges;
	std::vector<D3D12_STATIC_SAMPLER_DESC> m_static_samplers;

private:
	template<size_t root_arguments_byte_size>
	void CreateShaderRecord(ShaderRecordF<root_arguments_byte_size>& shader_record, const wchar_t* shader_name, void* root_argument_data)
	{
		ID3D12StateObjectProperties* stateObjectProperties = nullptr;
		HRESULT hr = m_raytracing_state_object->QueryInterface(IID_PPV_ARGS(&stateObjectProperties));
		assert(SUCCEEDED(hr));

		memcpy(shader_record.shader_identifier, stateObjectProperties->GetShaderIdentifier(shader_name),
			D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		memcpy(shader_record.root_arguments, root_argument_data, root_arguments_byte_size);

		stateObjectProperties->Release();
	}

private:
	ID3DBlob* LoadCSO(const std::string& filepath);
	void AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE range_type, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT descriptors = 1, UINT register_space = 0);
	void CreateRootSignature(ID3D12RootSignature** root_signature, bool global);
	RayShaderRecord CreateShaderRecord(UINT root_argument_size, const wchar_t* shader_name, void* root_argument_data);

public:
	dx12raytracingrenderpipeline();
	~dx12raytracingrenderpipeline();
	void CreateRayTracingStateObject(const std::string& shader_name, const std::wstring& hit_shader_name, UINT payload_size, UINT max_bounces);
	void CreateShaderRecordBuffers(const std::wstring& ray_generation_shader_name, const std::wstring& miss_shader_name, BufferResource triangle_colours, BufferResource view_projection_matrix);
	void CheckIfRaytracingRenderPipeline();
	ID3D12StateObject* GetRaytracingStateObject();
	ID3D12RootSignature* GetRaytracingLocalRootSignature();
	ID3D12RootSignature* GetRaytracingGlobalRootSignature();
	RayGenerationShaderRecordF<32>* RayGenerationShaderRecord();
	ShaderTableF<32, 1>* GetMissShaderRecord();
	ShaderTableF<32, 2>* GetHitShaderRecord();

	void AddConstantBuffer(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, D3D12_ROOT_PARAMETER_TYPE parameter_type = D3D12_ROOT_PARAMETER_TYPE_CBV, UINT register_space = 0);
	void AddStructuredBuffer(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, D3D12_DESCRIPTOR_RANGE_TYPE range_type = D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT register_space = 0);
	void AddShaderResource(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT descriptors = 1, UINT register_space = 0);
	void AddUnorderedAccess(UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);
	void AddStaticSampler(D3D12_FILTER sampler_filter, D3D12_TEXTURE_ADDRESS_MODE sampler_address_mode, UINT binding_slot, D3D12_SHADER_VISIBILITY shader_type, bool global, UINT register_space = 0);
};

