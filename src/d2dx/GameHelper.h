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

#include "IGameHelper.h"
#include "Types.h"

namespace d2dx 
{
	class GameHelper final : public IGameHelper
	{
	public:
		GameHelper();
		virtual ~GameHelper() noexcept {}

		virtual GameVersion GetVersion() const override;
		
		virtual _Ret_z_ const char* GetVersionString() const override;

		virtual uint32_t ScreenOpenMode() const override;
		
		virtual Size GetConfiguredGameSize() const override;
		
		virtual GameAddress IdentifyGameAddress(
			_In_ uint32_t returnAddress) const override;

		virtual TextureCategory GetTextureCategoryFromHash(
			_In_ uint32_t textureHash) const override;
		
		virtual TextureCategory RefineTextureCategoryFromGameAddress(
			_In_ TextureCategory previousCategory,
			_In_ GameAddress gameAddress) const override;

		virtual bool TryApplyInGameFpsFix() override;
		
		virtual bool TryApplyMenuFpsFix() override;

		virtual bool TryApplyInGameSleepFixes() override;

		virtual void* GetFunction(
			_In_ D2Function function) const override;

		virtual DrawParameters GetDrawParameters(
			_In_ const D2::CellContextAny* cellContext) const override;

		virtual D2::UnitAny* GetPlayerUnit() const override;

		virtual Offset GetUnitPos(
			_In_ const D2::UnitAny* unit) const override;

		virtual D2::UnitType GetUnitType(
			_In_ const D2::UnitAny* unit) const override;

		virtual uint32_t GetUnitId(
			_In_ const D2::UnitAny* unit) const override;

		virtual D2::UnitAny* FindUnit(
			_In_ uint32_t unitId,
			_In_ D2::UnitType unitType) const override;

		virtual int32_t GetCurrentAct() const override;

		virtual bool IsGameMenuOpen() const override;
		
		virtual bool IsInGame() const override;

		virtual bool IsProjectDiablo2() const override;

	private:
		GameVersion GetGameVersion();
		
		void InitializeTextureHashPrefixTable();
		
		bool ProbeUInt32(
			_In_ HANDLE hModule, 
			_In_ uint32_t offset,
			_In_ uint32_t expectedValue);

		void PatchUInt32(
			_In_ HANDLE hModule,
			_In_ uint32_t offset,
			_In_ uint32_t value);

		HANDLE _hProcess;
		HANDLE _hGameExe;
		HANDLE _hD2ClientDll;
		HANDLE _hD2CommonDll;
		HANDLE _hD2GfxDll;
		HANDLE _hD2WinDll;
		GameVersion _version;
		bool _isProjectDiablo2;
	};
}
