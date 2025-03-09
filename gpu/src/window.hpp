#pragma once

#include "../../common/constant.hpp"
#include "util.hpp"

#include <array>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#ifdef WINDOW_RENDERING

/// ウィンドウ(レンダーターゲット)を管理するオブジェクト
class WindowManager final {
private:
	const HINSTANCE _inst;
	const HWND _window;
	const ComPtr<IDXGISwapChain4> _swapChain;
	const ComPtr<ID3D12DescriptorHeap> _rtvHeap;
	const std::array<RenderTarget, FRAME_COUNT> _rts;

public:
	explicit WindowManager(HINSTANCE inst, const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12CommandQueue> &queue);
	WindowManager() = delete;
	WindowManager(const WindowManager &) = delete;
	WindowManager(const WindowManager &&) = delete;
	WindowManager &operator=(const WindowManager &) = delete;
	WindowManager &&operator=(const WindowManager &&) = delete;
	~WindowManager();

	inline bool process() const {
		MSG msg;
		while (true) {
			if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE) == 0) {
				return true;
			}
			if (msg.message == WM_QUIT) {
				return false;
			}
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	inline void attach(const ComPtr<ID3D12GraphicsCommandList> &cmdList) const {
		const auto index = _swapChain->GetCurrentBackBufferIndex();
		_rts[index].changeState(cmdList, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		_rts[index].attach(cmdList);
	}
	inline void detach(const ComPtr<ID3D12GraphicsCommandList>& cmdList) const {
		const auto index = _swapChain->GetCurrentBackBufferIndex();
		_rts[index].changeState(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}
	inline void present() const {
		_swapChain->Present(1, 0);
	}
};

#else

class WindowManager final {
public:
	explicit WindowManager(HINSTANCE, const ComPtr<ID3D12Device> &, const ComPtr<ID3D12CommandQueue> &) {}
	~WindowManager() = default;

	inline bool process() const {
		return true;
	}
	inline void attach(const ComPtr<ID3D12GraphicsCommandList> &) const {}
	inline void detach(const ComPtr<ID3D12GraphicsCommandList> &) const {}
	inline void present() const {}
};

#endif
