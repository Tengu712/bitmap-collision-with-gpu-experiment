#include "core.hpp"

namespace {
	inline ComPtr<ID3D12Device> createDevice() {
		ComPtr<IDXGIFactory6> factory;
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.GetAddressOf())))) {
			throw "failed to create a factory to select an adapter.";
		}
		ComPtr<IDXGIAdapter1> adapter;
		for (UINT i = 0; factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i) {
			DXGI_ADAPTER_DESC1 desc;
			if (FAILED(adapter->GetDesc1(&desc)) || desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
				continue;
			}
			ComPtr<ID3D12Device> device;
			if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(device.GetAddressOf())))) {
				continue;
			}
			return device;
		}
		throw "failed to create a device.";
	}

	inline ComPtr<ID3D12CommandQueue> createCommandQueue(const ComPtr<ID3D12Device> &device) {
		D3D12_COMMAND_QUEUE_DESC desc;
		desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;
		ComPtr<ID3D12CommandQueue> queue;
		if (FAILED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(queue.GetAddressOf())))) {
			throw "failed to create a command queue.";
		}
		return queue;
	}

	inline std::array<ComPtr<ID3D12CommandAllocator>, FRAME_COUNT> createCommandAllocator(const ComPtr<ID3D12Device> &device) {
		std::array<ComPtr<ID3D12CommandAllocator>, FRAME_COUNT> cmdAllocs;
		for (auto &n: cmdAllocs) {
			if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(n.GetAddressOf())))) {
				throw "failed to create a command allocator.";
			}
		}
		return cmdAllocs;
	}

	inline std::array<ComPtr<ID3D12GraphicsCommandList>, FRAME_COUNT> createCommandLists(
		const ComPtr<ID3D12Device> &device,
		const std::array<ComPtr<ID3D12CommandAllocator>, FRAME_COUNT> &cmdAllocs
	) {
		std::array<ComPtr<ID3D12GraphicsCommandList>, FRAME_COUNT> cmdLists;
		for (unsigned int i = 0; i < FRAME_COUNT; ++i) {
			if (FAILED(device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				cmdAllocs[i].Get(),
				nullptr,
				IID_PPV_ARGS(cmdLists[i].GetAddressOf())
			)))	{
				throw "failed to create a command list.";
			}
			cmdLists[i]->Close();
		}
		return cmdLists;
	}

	inline std::array<ComPtr<ID3D12Fence>, FRAME_COUNT> createFences(const ComPtr<ID3D12Device> &device) {
		std::array<ComPtr<ID3D12Fence>, FRAME_COUNT> fences;
		for (auto &n: fences) {
			if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(n.GetAddressOf())))) {
				throw "failed to create a fence.";
			}
		}
		return fences;
	}

	inline std::array<HANDLE, FRAME_COUNT> createFenceEvents() {
		std::array<HANDLE, FRAME_COUNT> fenceEvent;
		for (auto &n: fenceEvent) {
			n = CreateEventW(nullptr, FALSE, FALSE, nullptr);
			if (!n) {
				throw "failed to create an event.";
			}
		}
		return fenceEvent;
	}
}

Core::Core():
	_device(createDevice()),
	_queue(createCommandQueue(_device)),
	_cmdAllocs(createCommandAllocator(_device)),
	_cmdLists(createCommandLists(_device, _cmdAllocs)),
	_fences(createFences(_device)),
	_fenceEvents(createFenceEvents()),
	_fenceCounters{0, 0},
	_curFrameIndex(0)
{}

Core::~Core() {
	for (UINT i = 0; i < FRAME_COUNT; ++i) {
		CloseHandle(_fenceEvents[i]);
	}
}

void Core::wait() {
	if (_fences[_curFrameIndex]->GetCompletedValue() < _fenceCounters[_curFrameIndex]) {
		_fences[_curFrameIndex]->SetEventOnCompletion(_fenceCounters[_curFrameIndex], _fenceEvents[_curFrameIndex]);
		WaitForSingleObjectEx(_fenceEvents[_curFrameIndex], INFINITE, FALSE);
	}
	_fenceCounters[_curFrameIndex] += 1;
	_cmdAllocs[_curFrameIndex]->Reset();
	_cmdLists[_curFrameIndex]->Reset(_cmdAllocs[_curFrameIndex].Get(), nullptr);
}

void Core::submit() {
	_cmdLists[_curFrameIndex]->Close();
	std::array<ID3D12CommandList *const, 1> cmdLists{_cmdLists[_curFrameIndex].Get()};
	_queue->ExecuteCommandLists(static_cast<UINT>(cmdLists.size()), cmdLists.data());
}

void Core::next() {
	_queue->Signal(_fences[_curFrameIndex].Get(), _fenceCounters[_curFrameIndex]);
	_curFrameIndex = (_curFrameIndex + 1) % FRAME_COUNT;
}

void Core::waitAll() {
	for (UINT i = 0; i < FRAME_COUNT; ++i) {
		_fenceCounters[_curFrameIndex] += 1;
		_queue->Signal(_fences[_curFrameIndex].Get(), _fenceCounters[_curFrameIndex]);
		if (_fences[_curFrameIndex]->GetCompletedValue() < _fenceCounters[_curFrameIndex]) {
			_fences[_curFrameIndex]->SetEventOnCompletion(_fenceCounters[_curFrameIndex], _fenceEvents[_curFrameIndex]);
			WaitForSingleObjectEx(_fenceEvents[_curFrameIndex], INFINITE, FALSE);
		}
	}
}
