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

#include "IBuiltinResMod.h"
#include "IGameHelper.h"

namespace d2dx
{
    class BuiltinResMod final : public IBuiltinResMod
    {
    public:
        BuiltinResMod(
            _In_ HMODULE hModule,
            _In_ const std::shared_ptr<IGameHelper>& gameHelper);
        
        virtual ~BuiltinResMod() noexcept {}

        virtual bool IsActive() const override;

    private:
        bool IsCompatible(
            _In_ IGameHelper* gameHelper);

        void EnsureMpqExists(
            _In_ HMODULE hModule);

        bool _isActive = false;
    };
}
