#pragma once

#include "../../common/constant.hpp"
#include "util.hpp"

#include <array>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Bitmap final: public RenderTarget {
private:
	const ComPtr<ID3D12Resource> _rbb;
	const D3D12_TEXTURE_COPY_LOCATION _rbbCopyLocDst;
	const D3D12_TEXTURE_COPY_LOCATION _rbbCopyLocSrc;

public:
	explicit Bitmap(const ComPtr<ID3D12Device> &device, D3D12_CPU_DESCRIPTOR_HANDLE viewHandle):
		RenderTarget(
			device,
			createTexture2DResource(
				device,
				WIDTH,
				HEIGHT,
				createColorClearValue(CLEAR_COLOR),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
			),
			viewHandle
		),
		_rbb(createBufferResource(device, WIDTH * HEIGHT * 4, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST)),
		_rbbCopyLocDst{
			_rbb.Get(),
			D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
			{0, {DXGI_FORMAT_R8G8B8A8_UNORM, WIDTH, HEIGHT, 1, WIDTH * 4}},
		},
		_rbbCopyLocSrc{
			_res.Get(),
			D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			{0},
		}
	{}
	Bitmap() = delete;
	Bitmap(const Bitmap &) = delete;
	Bitmap(const Bitmap &&) = delete;
	Bitmap &operator=(const Bitmap &) = delete;
	Bitmap &&operator=(const Bitmap &&) = delete;
	~Bitmap() = default;

	inline void map(void **p) const {
		_rbb->Map(0, nullptr, p);
	}
	inline void unmap() const {
		_rbb->Unmap(0, nullptr);
	}
	inline void detach(const ComPtr<ID3D12GraphicsCommandList> &cmdList) const {
		RenderTarget::changeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
		cmdList->CopyTextureRegion(&_rbbCopyLocDst, 0, 0, 0, &_rbbCopyLocSrc, nullptr);
		RenderTarget::changeState(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
};

/// 衝突判定ビットマップ(レンダーターゲット)を管理するオブジェクト
class BitmapManager final {
private:
	const ComPtr<ID3D12DescriptorHeap> _rtvHeap;
	const std::array<Bitmap, FRAME_COUNT> _bitmaps;
	uint8_t *_mappedBitmap;

public:
	explicit BitmapManager(const ComPtr<ID3D12Device> &device):
		_rtvHeap(createRTVHeap(device)),
		_bitmaps{
			Bitmap(device, {_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr}),
			Bitmap(device, {_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)}),
		},
		_mappedBitmap(nullptr)
	{}
	BitmapManager() = delete;
	BitmapManager(const BitmapManager &) = delete;
	BitmapManager(const BitmapManager &&) = delete;
	BitmapManager &operator=(const BitmapManager &) = delete;
	BitmapManager &&operator=(const BitmapManager &&) = delete;
	~BitmapManager() = default;

	/// 衝突判定ビットマップをマップするためのメンバ関数
	///
	/// WARN: frameIndexには処理が終わったフレームの番号を指定すること。
	/// WARN: この関数を呼んだならば、後で必ずunmap()を呼ぶこと。
	inline void map(unsigned int frameIndex) {
		_bitmaps[frameIndex].map(reinterpret_cast<void **>(&_mappedBitmap));
	}

	/// 衝突判定ビットマップをアンマップするためのメンバ関数
	///
	/// WARN: この関数を呼ぶ前にmap()を呼んでおくこと。
	inline void unmap(unsigned int frameIndex) const {
		_bitmaps[frameIndex].unmap();
	}

	/// 衝突判定ビットマップ上に物体が存在するか確認する関数
	///
	/// WARN: この関数を呼ぶ前にmap()を呼んでおくこと。
	inline uint8_t check(int x, int y, int channel) const {
		if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
			return false;
		} else {
			return _mappedBitmap[4 * WIDTH * y + 4 * x + channel];
		}
	}

	/// 衝突判定ビットマップの描画を開始するためのメンバ関数
	inline void attach(const ComPtr<ID3D12GraphicsCommandList> &cmdList, unsigned int frameIndex) const {
		_bitmaps[frameIndex].attach(cmdList);
	}

	/// 衝突判定ビットマップの描画を終了するためのメンバ関数
	inline void detach(const ComPtr<ID3D12GraphicsCommandList> &cmdList, unsigned int frameIndex) const {
		_bitmaps[frameIndex].detach(cmdList);
	}
};
