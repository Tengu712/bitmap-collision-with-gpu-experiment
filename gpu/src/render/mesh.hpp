#pragma once

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct SquareMesh {
	const ComPtr<ID3D12Resource> vb;
	const ComPtr<ID3D12Resource> ib;
	const D3D12_VERTEX_BUFFER_VIEW vbv;
	const D3D12_INDEX_BUFFER_VIEW ibv;
	const UINT indexCount;

	explicit SquareMesh(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12CommandQueue> &queue);
	SquareMesh() = delete;
	SquareMesh(const SquareMesh &) = delete;
	SquareMesh(const SquareMesh &&) = delete;
	SquareMesh &operator=(const SquareMesh &) = delete;
	SquareMesh &&operator=(const SquareMesh &&) = delete;
	~SquareMesh() = default;
};
