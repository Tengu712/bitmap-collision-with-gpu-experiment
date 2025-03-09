#pragma once

#include "../../common/constant.hpp"

#include <array>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

/// Direct3D12の主要オブジェクトを管理するオブジェクト
class Core final {
private:
	const ComPtr<ID3D12Device> _device;
	const ComPtr<ID3D12CommandQueue> _queue;
	const std::array<ComPtr<ID3D12CommandAllocator>, FRAME_COUNT> _cmdAllocs;
	const std::array<ComPtr<ID3D12GraphicsCommandList>, FRAME_COUNT> _cmdLists;
	const std::array<ComPtr<ID3D12Fence>, FRAME_COUNT> _fences;
	std::array<HANDLE, FRAME_COUNT> _fenceEvents;
	std::array<UINT64, FRAME_COUNT> _fenceCounters;
	UINT _curFrameIndex;

public:
	explicit Core();
	Core(const Core &) = delete;
	Core(const Core &&) = delete;
	Core &operator=(const Core &) = delete;
	Core &&operator=(const Core &&) = delete;
	~Core();

	inline const ComPtr<ID3D12Device> &getDevice() const {
		return _device;
	}
	inline const ComPtr<ID3D12CommandQueue>& getQueue() const {
		return _queue;
	}
	inline const ComPtr<ID3D12GraphicsCommandList> &getCurrentCommandList() const {
		return _cmdLists.at(_curFrameIndex);
	}
	inline UINT getCurrentFrameIndex() const {
		return _curFrameIndex;
	}

	/// 現在のフレームのコマンドキューが空になるまで待機する関数
	void wait();

	/// 現在のフレームのコマンドリストをコマンドキューに提出する関数
	///
	/// WARN: この関数を呼んでからnext()を呼ぶまでの間にコマンドリストにコマンドを追加しないこと。
	void submit();

	/// 次のフレームに移る関数
	void next();

	/// すべてのコマンドキューが空になるまで待機する関数
	///
	/// 主にDirect3D12を終了するときに用いる。
	void waitAll();
};
