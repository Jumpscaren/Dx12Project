#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <assert.h>
#include <wrl/client.h>
#include <vector>
#include <string>

class dx12renderpipeline
{
private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline_state;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature;
};

