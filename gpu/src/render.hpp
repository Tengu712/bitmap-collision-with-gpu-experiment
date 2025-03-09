#pragma once

#include "../../common/constant.hpp"
#include "render/mesh.hpp"
#include "render/texture.hpp"
#include "util.hpp"

#include <array>
#include <DirectXMath.h>
#include <vector>

struct EntityDataLayout {
	DirectX::XMFLOAT4 trans;
	DirectX::XMFLOAT4 scale;
	DirectX::XMFLOAT4 offset;
};

/// 描画を行うオブジェクト
class Renderer final {
private:
	const ComPtr<ID3D12RootSignature> _rootSig;
	const ComPtr<ID3D12PipelineState> _state;
	const D3D12_VIEWPORT _viewport;
	const D3D12_RECT _scissor;
	const ComPtr<ID3D12DescriptorHeap> _srvHeap;
	const std::array<ComPtr<ID3D12Resource>, FRAME_COUNT> _entities;
	const ComPtr<ID3D12Resource> _camera;
	const CircleTexture _texture;
	const SquareMesh _mesh;

public:
	explicit Renderer(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12CommandQueue> &queue, UINT instCount);
	Renderer() = delete;
	Renderer(const Renderer &) = delete;
	Renderer(const Renderer &&) = delete;
	Renderer &operator=(const Renderer &) = delete;
	Renderer &&operator=(const Renderer &&) = delete;
	~Renderer() = default;

	/// entitiesを更新する関数
	///
	/// WARN: 一度にすべてのデータが転送されることを想定している。
	inline void uploadEntities(UINT frameIndex, const std::vector<EntityDataLayout> &data) const {
		uploadToUploadHeap(_entities[frameIndex], static_cast<const void *>(data.data()), sizeof(EntityDataLayout) * data.size());
	}

	/// 描画を行うメンバ関数
	///
	/// WARN: 一フレームに一度しか呼ばれない想定をしている。
	inline void draw(const ComPtr<ID3D12GraphicsCommandList> &cmdList, UINT frameIndex, UINT instCount) const {
		std::array<ID3D12DescriptorHeap *, 1> descHeaps{_srvHeap.Get()};
		cmdList->SetDescriptorHeaps(static_cast<UINT>(descHeaps.size()), descHeaps.data());
		cmdList->SetGraphicsRootSignature(_rootSig.Get());
		cmdList->SetGraphicsRootShaderResourceView(0, _entities[frameIndex]->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootConstantBufferView(1, _camera->GetGPUVirtualAddress());
		cmdList->SetGraphicsRootDescriptorTable(2, _srvHeap->GetGPUDescriptorHandleForHeapStart());

		cmdList->SetPipelineState(_state.Get());
		cmdList->RSSetViewports(1, &_viewport);
		cmdList->RSSetScissorRects(1, &_scissor);

		cmdList->IASetVertexBuffers(0, 1, &_mesh.vbv);
		cmdList->IASetIndexBuffer(&_mesh.ibv);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		cmdList->DrawIndexedInstanced(_mesh.indexCount, instCount, 0, 0, 0);
	}
};
