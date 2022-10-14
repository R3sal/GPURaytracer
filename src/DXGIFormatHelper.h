#pragma once

#include <dxgi1_6.h>



namespace DirectX
{
	bool IsPacked(DXGI_FORMAT fmt) noexcept;
	bool IsVideo(DXGI_FORMAT fmt) noexcept;
	bool IsPlanar(DXGI_FORMAT fmt) noexcept;
	bool IsDepthStencil(DXGI_FORMAT fmt) noexcept;
	bool IsTypeless(DXGI_FORMAT fmt, bool partialTypeless) noexcept;
	bool HasAlpha(DXGI_FORMAT fmt) noexcept;
	size_t BitsPerPixel(DXGI_FORMAT fmt) noexcept;
	size_t BitsPerColor(DXGI_FORMAT fmt) noexcept;
}