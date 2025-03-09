#pragma once

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct CircleTexture {
	const ComPtr<ID3D12Resource> res;

	explicit CircleTexture(
		const ComPtr<ID3D12Device> &device,
		const ComPtr<ID3D12CommandQueue> &queue,
		D3D12_CPU_DESCRIPTOR_HANDLE handle
	);
	CircleTexture() = delete;
	CircleTexture(const CircleTexture &) = delete;
	CircleTexture(const CircleTexture &&) = delete;
	CircleTexture &operator=(const CircleTexture &) = delete;
	CircleTexture &&operator=(const CircleTexture &&) = delete;
	~CircleTexture() = default;
};
