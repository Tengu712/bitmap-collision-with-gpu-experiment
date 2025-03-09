#include "window.hpp"

#include <iostream>

#ifdef WINDOW_RENDERING

namespace {
	constexpr LPCWSTR WINDOW_CLASS_NAME = L"BCWGE_WC";

	LRESULT CALLBACK windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
			default:
				return DefWindowProc(window, msg, wParam, lParam);
		}
	}

	inline const HWND createWindow(HINSTANCE inst) {
		constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

		WNDCLASSEXW windowClass;
		windowClass.cbSize        = sizeof(WNDCLASSEXW);
		windowClass.style         = CS_CLASSDC;
		windowClass.lpfnWndProc   = windowProc;
		windowClass.cbClsExtra    = 0;
		windowClass.cbWndExtra    = 0;
		windowClass.hInstance     = inst;
		windowClass.hIcon         = nullptr;
		windowClass.hCursor       = nullptr;
		windowClass.hbrBackground = nullptr;
		windowClass.lpszMenuName  = nullptr;
		windowClass.lpszClassName = WINDOW_CLASS_NAME;
		windowClass.hIconSm       = nullptr;
		if (RegisterClassExW(&windowClass) == 0) {
			throw "failed to register a window class.";
		}

		RECT rect = { 0, 0, WIDTH, HEIGHT };
		AdjustWindowRect(&rect, style, 0);

		const auto window = CreateWindowExW(
			0,
			WINDOW_CLASS_NAME,
			WINDOW_CLASS_NAME,
			style,
			0,
			0,
			rect.right  - rect.left,
			rect.bottom - rect.top,
			NULL,
			NULL,
			inst,
			NULL
		);
		if (window == nullptr) {
			throw "failed to create a window.";
		}
		ShowWindow(window, SW_SHOWDEFAULT);
		UpdateWindow(window);

		return window;
	}

	inline ComPtr<IDXGISwapChain4> createSwapChain(const ComPtr<ID3D12CommandQueue> &queue, const HWND window) {
		ComPtr<IDXGIFactory5> factory;
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.GetAddressOf())))) {
			throw "failed to create a factory to create a swap chain.";
		}
		DXGI_SWAP_CHAIN_DESC desc;
		desc.BufferDesc.Width                   = WIDTH;
		desc.BufferDesc.Height                  = HEIGHT;
		desc.BufferDesc.RefreshRate.Numerator   = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count   = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount  = FRAME_COUNT;
		desc.OutputWindow = window;
		desc.Windowed     = TRUE;
		desc.SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		ComPtr<IDXGISwapChain> predSwapChain;
		if (FAILED(factory->CreateSwapChain(queue.Get(), &desc, predSwapChain.GetAddressOf()))) {
			throw "failed to create a predecessor of a swap chain.";
		}
		ComPtr<IDXGISwapChain4> swapChain;
		if (FAILED(predSwapChain->QueryInterface(IID_PPV_ARGS(swapChain.GetAddressOf())))) {
			throw "failed to get IDXGISwapChain4 from IDXGISwapChain.";
		}
		return swapChain;
	}

	inline ComPtr<ID3D12Resource> getBackBuffer(UINT index, const ComPtr<IDXGISwapChain4> &swapChain) {
		ComPtr<ID3D12Resource> backBuffer;
		if (FAILED(swapChain->GetBuffer(index, IID_PPV_ARGS(backBuffer.GetAddressOf())))) {
			throw "failed to get a back buffer.";
		}
		return backBuffer;
	}
}

WindowManager::WindowManager(HINSTANCE inst, const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12CommandQueue> &queue):
	_inst(inst),
	_window(createWindow(inst)),
	_swapChain(createSwapChain(queue, _window)),
	_rtvHeap(createRTVHeap(device)),
	_rts{
		RenderTarget(device, getBackBuffer(0, _swapChain), {_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr}),
		RenderTarget(device, getBackBuffer(1, _swapChain), {_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)}),
	}
{}

WindowManager::~WindowManager() {
	UnregisterClassW(WINDOW_CLASS_NAME, _inst);
}

#endif
