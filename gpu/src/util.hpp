#pragma once

#include "../../common/common.hpp"

#include <array>
#include <d3d12.h>
#include <functional>
#include <optional>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

inline D3D12_CLEAR_VALUE createColorClearValue(const std::array<FLOAT, 4> &color) {
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	std::copy(color.cbegin(), color.cend(), clearValue.Color);
	return clearValue;
}

inline D3D12_HEAP_PROPERTIES createHeapProperties(D3D12_HEAP_TYPE type) {
	return {
		type,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		1,
		1,
	};
}

inline D3D12_RESOURCE_DESC createBufferResourceDesc(UINT size) {
	return {
		D3D12_RESOURCE_DIMENSION_BUFFER,
		0,
		(size + 255) & ~255,
		1,
		1,
		1,
		DXGI_FORMAT_UNKNOWN,
		{1, 0},
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_RESOURCE_FLAG_NONE,
	};
}

inline D3D12_RESOURCE_DESC createTexture2DResourceDesc(UINT width, UINT height, D3D12_RESOURCE_FLAGS flag) {
	return {
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		width,
		height,
		1,
		1,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		{1, 0},
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		flag,
	};
}

inline ComPtr<ID3D12Resource> createBufferResource(
	const ComPtr<ID3D12Device> &device,
	UINT size,
	D3D12_HEAP_TYPE type = D3D12_HEAP_TYPE_UPLOAD,
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ
) {
	const D3D12_HEAP_PROPERTIES prop = createHeapProperties(type);
	const D3D12_RESOURCE_DESC desc = createBufferResourceDesc(size);
	ComPtr<ID3D12Resource> res;
	if (FAILED(device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		state,
		nullptr,
		IID_PPV_ARGS(res.GetAddressOf())
	))) {
		throw "failed to create a buffer resource.";
	}
	return res;
}

inline ComPtr<ID3D12Resource> createTexture2DResource(
	const ComPtr<ID3D12Device> &device,
	UINT width,
	UINT height,
	std::optional<D3D12_CLEAR_VALUE> clearValue,
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ,
	D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE
) {
	const D3D12_HEAP_PROPERTIES prop = createHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const D3D12_RESOURCE_DESC desc = createTexture2DResourceDesc(width, height, flag);
	ComPtr<ID3D12Resource> res;
	if (FAILED(device->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		state,
		clearValue ? &clearValue.value() : nullptr,
		IID_PPV_ARGS(res.GetAddressOf())
	))) {
		throw "failed to create a texture2d resource.";
	}
	return res;
}

inline void pushTransitionBarrier(
	const ComPtr<ID3D12GraphicsCommandList> &cmdList,
	const ComPtr<ID3D12Resource> &res,
	D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after
) {
	D3D12_RESOURCE_BARRIER desc;
	desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	desc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	desc.Transition.pResource   = res.Get();
	desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	desc.Transition.StateBefore = before;
	desc.Transition.StateAfter  = after;
	cmdList->ResourceBarrier(1, &desc);
}

inline void uploadToUploadHeap(const ComPtr<ID3D12Resource> &res, const void *src, size_t size) {
	void *p;
	if (FAILED(res->Map(0, nullptr, &p))) {
		throw "failed to map a resource on a upload heap.";
	}
	memcpy(p, src, size);
	res->Unmap(0, nullptr);
}

inline void uploadToDefaultHeapImmediately(
	const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12CommandQueue>& queue,
	const ComPtr<ID3D12Resource> &dst,
	const void *src,
	size_t size,
	std::function<void(const ComPtr<ID3D12GraphicsCommandList> &, const ComPtr<ID3D12Resource> &)> f
) {
	// ステージングバッファ作成・アップロード
	const auto stage = createBufferResource(device, static_cast<UINT>(size));
	uploadToUploadHeap(stage, src, size);

	// コマンドアロケータ・コマンドリスト作成
	ComPtr<ID3D12CommandAllocator> cmdAlloc;
	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(cmdAlloc.GetAddressOf()));
	ComPtr<ID3D12GraphicsCommandList> cmdList;
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_PPV_ARGS(cmdList.GetAddressOf()));

	// ステージングバッファから目的バッファへアップロード
	pushTransitionBarrier(cmdList, dst, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
	f(cmdList, stage);
	pushTransitionBarrier(cmdList, dst, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);

	// コマンド実行
	cmdList->Close();
	ID3D12CommandList* cmdLists[]{cmdList.Get()};
	queue->ExecuteCommandLists(1, cmdLists);

	// フェンス・イベントハンドル作成
	ComPtr<ID3D12Fence> fence;
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
	HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!eventHandle) {
		throw "failed to create an event handle to wait for copying data from upload heap to default heap.";
	}

	// コマンド実行完了まで待機
	queue->Signal(fence.Get(), 1);
	if (fence->GetCompletedValue() < 1) {
		fence->SetEventOnCompletion(1, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
	}
	CloseHandle(eventHandle);
}

inline void uploadToBufferOnDefaultHeapImmediately(
	const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12CommandQueue>& queue,
	const ComPtr<ID3D12Resource> &dst,
	const void *src,
	size_t size
) {
	uploadToDefaultHeapImmediately(
		device,
		queue,
		dst,
		src,
		size,
		[&dst](const ComPtr<ID3D12GraphicsCommandList> &cmdList, const ComPtr<ID3D12Resource> &stage) {
			cmdList->CopyResource(dst.Get(), stage.Get());
		}
	);
}

inline void uploadToTexture2DOnDefaultHeapImmediately(
	const ComPtr<ID3D12Device>& device,
	const ComPtr<ID3D12CommandQueue>& queue,
	const ComPtr<ID3D12Resource> &dst,
	const void *src,
	UINT width,
	UINT height
) {
	const D3D12_RESOURCE_DESC desc = createTexture2DResourceDesc(width, height, D3D12_RESOURCE_FLAG_NONE);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrint;
	UINT64 size;
	device->GetCopyableFootprints(&desc, 0, 1, 0, &footPrint, nullptr, nullptr, &size);

	uploadToDefaultHeapImmediately(
		device,
		queue,
		dst,
		src,
		size,
		[&dst, &footPrint](const ComPtr<ID3D12GraphicsCommandList> &cmdList, const ComPtr<ID3D12Resource> &stage) {
			D3D12_TEXTURE_COPY_LOCATION dstLoc;
			dstLoc.pResource        = dst.Get();
			dstLoc.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLoc.SubresourceIndex = 0;
			D3D12_TEXTURE_COPY_LOCATION srcLoc;
			srcLoc.pResource       = stage.Get();
			srcLoc.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLoc.PlacedFootprint = footPrint;
			cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
		}
	);
}

inline ComPtr<ID3D12DescriptorHeap> createRTVHeap(const ComPtr<ID3D12Device> &device) {
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	desc.NumDescriptors = FRAME_COUNT;
	desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask       = 0;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(rtvHeap.GetAddressOf())))) {
		throw "failed to create a descriptor heap for render target views.";
	}
	return rtvHeap;
}

inline void createRTV(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12Resource> &res, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle) {
	D3D12_RENDER_TARGET_VIEW_DESC desc;
	desc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice   = 0;
	desc.Texture2D.PlaneSlice = 0;
	device->CreateRenderTargetView(res.Get(), &desc, viewHandle);
}

class RenderTarget {	
protected:
	static constexpr std::array<float, 4> CLEAR_COLOR{0.0f, 0.0f, 0.0f, 0.0f};
	const ComPtr<ID3D12Resource> _res;
	const D3D12_CPU_DESCRIPTOR_HANDLE _viewHandle;

public:
	explicit RenderTarget(const ComPtr<ID3D12Device> &device, ComPtr<ID3D12Resource> &&res, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle):
		_res(std::move(res)),
		_viewHandle(viewHandle)
	{
		createRTV(device, _res, viewHandle);
	}
	RenderTarget() = delete;
	RenderTarget(const RenderTarget &) = delete;
	RenderTarget(const RenderTarget &&) = delete;
	RenderTarget &operator=(const RenderTarget &) = delete;
	RenderTarget &&operator=(const RenderTarget &&) = delete;
	virtual ~RenderTarget() = default;

	inline void changeState(const ComPtr<ID3D12GraphicsCommandList> &cmdList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) const {
		pushTransitionBarrier(cmdList, _res, before, after);
	}
	inline void attach(const ComPtr<ID3D12GraphicsCommandList> &cmdList) const {
		cmdList->ClearRenderTargetView(_viewHandle, CLEAR_COLOR.data(), 0, nullptr);
		cmdList->OMSetRenderTargets(1, &_viewHandle, FALSE, nullptr);
	}
};
