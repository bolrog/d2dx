/*
	This file is part of D2DX.

	Copyright (C) 2021  Bolrog

	D2DX is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	D2DX is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with D2DX.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once
#include <stdint.h>
#include <Unknwn.h>

#ifdef D2DX_EXPORT
#define D2DX_EXPORTED __declspec(dllexport)
#else
#define D2DX_EXPORTED
#endif

/*
	ID2DXConfigurator is a pseudo-COM interface that can be used to integrate with D2DX.
	It is suitable e.g. for "high-res" mods that want to use arbitrary resolutions.

	It can be accessed via the helper d2dx::TryGetConfigurator, or manually by 
	using LoadLibrary/GetProcAddress to get a pointer to _D2DXGetConfigurator@4.

	Note that the object is not really refcounted and there's no need to call AddRef/Release.

	Example use:

		ID2DXConfigurator* d2dxConfigurator = d2dx::TryGetConfigurator();
		if (d2dxConfigurator)
		{
			d2dxConfigurator->SetCustomResolution(1000, 500);
		}	
		grSstWinOpen(hWnd, ...)

*/
MIDL_INTERFACE("B11C5FA4-983F-4E34-9E43-BD82F9CCDB65")
ID2DXConfigurator : public IUnknown
{
public:
	/*
		Tell D2DX that the following call(s) to grSstWinOpen intends this custom resolution,
		and to ignore the 'screen_resolution' argument. To return to normal behavior, call
		this method with a width and height of zero.

		Returns S_OK on success and an HRESULT error code otherwise.
	*/
	virtual HRESULT STDMETHODCALLTYPE SetCustomResolution(
		int width,
		int height) = 0;

	/*
		Get a suggested custom resolution from D2DX (typically close to 640x480 or 800x600,
		but in the aspect ratio of the monitor).

		This method allows matching D2DX's default behavior.

		Returns S_OK on success and an HRESULT error code otherwise.
	*/
	virtual HRESULT STDMETHODCALLTYPE GetSuggestedCustomResolution(
		/* [out] */ int* width,
		/* [out] */ int* height) = 0;
};

extern "C"
{
	D2DX_EXPORTED ID2DXConfigurator* __stdcall D2DXGetConfigurator();
}

#if __cplusplus > 199711L
namespace d2dx
{
	namespace detail
	{
		class D2DXGetConfiguratorFuncLoader final
		{
		public:
			D2DXGetConfiguratorFuncLoader() : _d2dxGetConfiguratorFunc{ nullptr }
			{
				HINSTANCE hD2DX = LoadLibraryA("glide3x.dll");
				if (hD2DX)
				{
					_d2dxGetConfiguratorFunc = (D2DXGetConfiguratorFunc)GetProcAddress(
						hD2DX, "_D2DXGetConfigurator@4");
				}
			}
			ID2DXConfigurator* operator()()
			{
				return _d2dxGetConfiguratorFunc ? _d2dxGetConfiguratorFunc() : nullptr;
			}
		private:
			typedef ID2DXConfigurator* (*__stdcall D2DXGetConfiguratorFunc)();
			D2DXGetConfiguratorFunc _d2dxGetConfiguratorFunc;
		};
	}
	static ID2DXConfigurator* TryGetConfigurator()
	{
		static detail::D2DXGetConfiguratorFuncLoader d2dxGetConfiguratorFuncLoader;
		return d2dxGetConfiguratorFuncLoader();
	}
}
#endif
