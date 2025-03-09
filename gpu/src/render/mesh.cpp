#include "mesh.hpp"

#include "../util.hpp"

namespace {
	struct Vertex {
		float x, y, z, w, u, v;
	};

	constexpr std::array<Vertex, 4> VERTICES{
		Vertex{-0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
		Vertex{ 0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f},
		Vertex{-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
		Vertex{ 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f},
	};
	constexpr UINT VERTICES_SIZE = static_cast<UINT>(sizeof(Vertex) * VERTICES.size());

	constexpr std::array<SHORT, 4> INDICES{0, 1, 2, 3};
	constexpr UINT INDICES_SIZE = static_cast<UINT>(sizeof(SHORT) * INDICES.size());
}

SquareMesh::SquareMesh(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12CommandQueue> &queue):
	vb(createBufferResource(device, VERTICES_SIZE, D3D12_HEAP_TYPE_DEFAULT)),
	ib(createBufferResource(device, INDICES_SIZE,  D3D12_HEAP_TYPE_DEFAULT)),
	vbv{vb->GetGPUVirtualAddress(), VERTICES_SIZE, sizeof(Vertex)},
	ibv{ib->GetGPUVirtualAddress(), INDICES_SIZE, DXGI_FORMAT_R16_UINT},
	indexCount(static_cast<UINT>(INDICES.size()))
{
	uploadToBufferOnDefaultHeapImmediately(device, queue, vb, static_cast<const void *>(VERTICES.data()), VERTICES_SIZE);
	uploadToBufferOnDefaultHeapImmediately(device, queue, ib, static_cast<const void *>(INDICES.data()),  INDICES_SIZE);
}
