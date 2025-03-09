#include "render.hpp"

#include <d3dcompiler.h>

namespace {
	struct CameraDataLayout final {
		DirectX::XMMATRIX proj;
	};

	inline ComPtr<ID3D12RootSignature> createRootSignature(const ComPtr<ID3D12Device> &device) {
		std::array<D3D12_ROOT_PARAMETER, 3> params;
		// entities
		params[0].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_SRV;
		params[0].Descriptor.ShaderRegister = 0;
		params[0].Descriptor.RegisterSpace  = 0;
		params[0].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;
		// camera
		params[1].ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
		params[1].Descriptor.ShaderRegister = 0;
		params[1].Descriptor.RegisterSpace  = 0;
		params[1].ShaderVisibility          = D3D12_SHADER_VISIBILITY_VERTEX;
		// tex
		D3D12_DESCRIPTOR_RANGE range;
		range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		range.NumDescriptors                    = 1;
		range.BaseShaderRegister                = 1;
		range.RegisterSpace                     = 0;
		range.OffsetInDescriptorsFromTableStart = 0;
		params[2].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		params[2].DescriptorTable.NumDescriptorRanges = 1;
		params[2].DescriptorTable.pDescriptorRanges   = &range;
		params[2].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_PIXEL;
		// smplr
		D3D12_STATIC_SAMPLER_DESC smplr;
		smplr.Filter           = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		smplr.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		smplr.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		smplr.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		smplr.MipLODBias       = D3D12_DEFAULT_MIP_LOD_BIAS;
		smplr.MaxAnisotropy    = 1;
		smplr.ComparisonFunc   = D3D12_COMPARISON_FUNC_NEVER;
		smplr.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		smplr.MinLOD           = -D3D12_FLOAT32_MAX;
		smplr.MaxLOD           = +D3D12_FLOAT32_MAX;
		smplr.ShaderRegister   = 0;
		smplr.RegisterSpace    = 0;
		smplr.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC desc;
		desc.NumParameters     = static_cast<UINT>(params.size());
		desc.pParameters       = params.data();
		desc.NumStaticSamplers = 1;
		desc.pStaticSamplers   = &smplr;
		desc.Flags             =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
			| D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		ComPtr<ID3DBlob> blob;
		ComPtr<ID3DBlob> errorBlob;
		if (FAILED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, blob.GetAddressOf(), errorBlob.GetAddressOf()))) {
			throw "failed to serialize a root signature.";
		}
		ComPtr<ID3D12RootSignature> rootSig;
		if (FAILED(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(rootSig.GetAddressOf())))) {
			throw "failed to create a root signature.";
		}
		return rootSig;
	}

	inline ComPtr<ID3D12PipelineState> createPipelineState(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12RootSignature> &rootSig) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

		ComPtr<ID3DBlob> vs;
		ComPtr<ID3DBlob> ps;
		if (FAILED(D3DReadFileToBlob(L"vs.cso", vs.GetAddressOf()))) {
			throw "failed to load vs.cso.";
		}
		if (FAILED(D3DReadFileToBlob(L"ps.cso", ps.GetAddressOf()))) {
			throw "failed to load ps.cso.";
		}
		desc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
		desc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};

		desc.BlendState.AlphaToCoverageEnable  = FALSE;
		desc.BlendState.IndependentBlendEnable = FALSE;
		desc.BlendState.RenderTarget[0].BlendEnable           = TRUE;
		desc.BlendState.RenderTarget[0].LogicOpEnable         = FALSE;
		desc.BlendState.RenderTarget[0].SrcBlend              = D3D12_BLEND_ONE;
		desc.BlendState.RenderTarget[0].DestBlend             = D3D12_BLEND_ONE;
		desc.BlendState.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
		desc.BlendState.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
		desc.BlendState.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_ONE;
		desc.BlendState.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
		desc.BlendState.RenderTarget[0].LogicOp               = D3D12_LOGIC_OP_NOOP;
		desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		desc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
		desc.RasterizerState.CullMode              = D3D12_CULL_MODE_NONE;
		desc.RasterizerState.FrontCounterClockwise = FALSE;
		desc.RasterizerState.DepthBias             = D3D12_DEFAULT_DEPTH_BIAS;
		desc.RasterizerState.DepthBiasClamp        = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		desc.RasterizerState.SlopeScaledDepthBias  = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		desc.RasterizerState.DepthClipEnable       = FALSE;
		desc.RasterizerState.MultisampleEnable     = FALSE;
		desc.RasterizerState.AntialiasedLineEnable = FALSE;
		desc.RasterizerState.ForcedSampleCount     = 0;
		desc.RasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		desc.DepthStencilState.DepthEnable      = FALSE;
		desc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
		desc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[2];
		inputElementDescs[0].SemanticName         = "POSITION";
		inputElementDescs[0].SemanticIndex        = 0;
		inputElementDescs[0].Format               = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].InputSlot            = 0;
		inputElementDescs[0].AlignedByteOffset    = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[0].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputElementDescs[0].InstanceDataStepRate = 0;
		inputElementDescs[1].SemanticName         = "TEXCOORD";
		inputElementDescs[1].SemanticIndex        = 0;
		inputElementDescs[1].Format               = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].InputSlot            = 0;
		inputElementDescs[1].AlignedByteOffset    = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[1].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		inputElementDescs[1].InstanceDataStepRate = 0;
		desc.InputLayout           = {inputElementDescs, 2};

		desc.pRootSignature        = rootSig.Get();
		desc.SampleMask            = UINT_MAX;
		desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		desc.NumRenderTargets      = 1;
		desc.RTVFormats[0]         = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.DSVFormat             = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc            = {1, 0};

		ComPtr<ID3D12PipelineState> state;
		if (FAILED(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(state.GetAddressOf())))) {
			throw "failed to create a pipeline state.";
		}
		return state;
	}

	inline ComPtr<ID3D12DescriptorHeap> createSRVHeap(const ComPtr<ID3D12Device> &device) {
		D3D12_DESCRIPTOR_HEAP_DESC desc;
		desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask       = 0;
		ComPtr<ID3D12DescriptorHeap> descHeap;
		if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(descHeap.GetAddressOf())))) {
			throw "failed to create a descriptor heap for a texture.";
		}
		return descHeap;
	}
}

Renderer::Renderer(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12CommandQueue> &queue, UINT instCount):
	_rootSig(createRootSignature(device)),
	_state(createPipelineState(device, _rootSig)),
	_viewport{0.0f, 0.0f, WIDTH_FLOAT, HEIGHT_FLOAT, 0.0f, 1.0f},
	_scissor{0, 0, WIDTH, HEIGHT},
	_srvHeap(createSRVHeap(device)),
	_entities{
		createBufferResource(device, sizeof(EntityDataLayout) * instCount),
		createBufferResource(device, sizeof(EntityDataLayout) * instCount),
	},
	_camera(createBufferResource(device, sizeof(CameraDataLayout), D3D12_HEAP_TYPE_DEFAULT)),
	_texture(device, queue, _srvHeap->GetCPUDescriptorHandleForHeapStart()),
	_mesh(device, queue)
{
	const CameraDataLayout camera{
		DirectX::XMMatrixOrthographicOffCenterLH(0.0f, WIDTH_FLOAT, HEIGHT_FLOAT, 0.0f, 0.0f, 1.0f),
	};
	uploadToBufferOnDefaultHeapImmediately(device, queue, _camera, static_cast<const void *>(&camera), sizeof(CameraDataLayout));
}
