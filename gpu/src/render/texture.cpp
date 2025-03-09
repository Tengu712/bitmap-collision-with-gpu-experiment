#include "texture.hpp"

#include "../util.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {
	inline ComPtr<ID3D12Resource> createResource(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12CommandQueue> &queue) {
		int width, height, channels;
		unsigned char *imageData = stbi_load("circle.png", &width, &height, &channels, 4);
		if (!imageData) {
			throw "failed to load circle.png.";
		}

		const auto res = createTexture2DResource(device, width, height, std::nullopt);
		uploadToTexture2DOnDefaultHeapImmediately(device, queue, res, static_cast<const void *>(imageData), width, height);

		stbi_image_free(imageData);
		return res;
	}
}

CircleTexture::CircleTexture(
	const ComPtr<ID3D12Device> &device,
	const ComPtr<ID3D12CommandQueue> &queue,
	D3D12_CPU_DESCRIPTOR_HANDLE handle
):
	res(createResource(device, queue))
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc;
	desc.Format                        = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	desc.Texture2D.MostDetailedMip     = 0;
	desc.Texture2D.MipLevels           = 1;
	desc.Texture2D.PlaneSlice          = 0;
	desc.Texture2D.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(res.Get(), &desc, handle);
}
