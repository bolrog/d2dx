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
#include "pch.h"
#include "D2DXContextFactory.h"
#include "D2DXConfigurator.h"

using namespace d2dx;
using namespace std;
using namespace Microsoft::WRL;

class D2DXConfigurator : public ID2DXConfigurator
{
public:
    D2DXConfigurator() noexcept
    {
    }

    virtual ~D2DXConfigurator() noexcept
    {
    }

    STDMETHOD(QueryInterface)(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject)
    {
        if (IsEqualIID(riid, __uuidof(IUnknown)) ||
            IsEqualIID(riid, __uuidof(ID2DXConfigurator)))
        {
            *ppvObject = this;
            return S_OK;
        }
        else
        {
            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }
    }

    STDMETHOD_(ULONG, AddRef)(void)
    {
        /* We just pretend to be refcounted. */
        return 1U;
    }

    STDMETHOD_(ULONG, Release)(void)
    {
        /* We just pretend to be refcounted. */
        return 1U;
    }

    STDMETHOD(SetCustomResolution)(
        int32_t width,
        int32_t height) noexcept
    {
        auto d2dxContext = D2DXContextFactory::GetInstance(false);

        if (!d2dxContext)
        {
            return E_POINTER;
        }

        d2dxContext->SetCustomResolution({ width, height });

        return S_OK;
    }

    STDMETHOD(GetSuggestedCustomResolution)(
        _Out_ int32_t* width,
        _Out_ int32_t* height) noexcept
    {
        if (!width || !height)
        {
            return E_INVALIDARG;
        }

        auto d2dxContext = D2DXContextFactory::GetInstance(false);

        if (!d2dxContext)
        {
            return E_POINTER;
        }

        Size size = d2dxContext->GetSuggestedCustomResolution();
        *width = size.width;
        *height = size.height;

        return S_OK;
    }
};

namespace d2dx
{
    ID2DXConfigurator* GetConfiguratorInternal()
    {
        static D2DXConfigurator configurator;
        return &configurator;
    }
}

extern "C"
{
    D2DX_EXPORTED ID2DXConfigurator* __stdcall D2DXGetConfigurator()
    {
        auto d2dxInstance = D2DXContextFactory::GetInstance(false);

        if (!d2dxInstance)
        {
            return nullptr;
        }

        d2dxInstance->DisableBuiltinResMod();

        return GetConfiguratorInternal();
    }
}
