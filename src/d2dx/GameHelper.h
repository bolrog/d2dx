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
	class GameHelper final : public RuntimeClass<
		RuntimeClassFlags<RuntimeClassType::ClassicCom>,
		IGameHelper
	>
	{
	public:
		GameHelper();
		virtual ~GameHelper();

		virtual GameVersion GetVersion() const override;
		
		virtual _Ret_z_ const char* GetVersionString() const override;

		virtual uint32_t ScreenOpenMode() const override;
		
		virtual Size GetConfiguredGameSize() const override;
		
		virtual void SetIngameMousePos(
			Offset pos) override;

		virtual GameAddress IdentifyGameAddress(
			uint32_t returnAddress) const override;

		virtual TextureCategory GetTextureCategoryFromHash(
			uint32_t textureHash) const override;
		
		_Use_decl_annotations_
		virtual TextureCategory RefineTextureCategoryFromGameAddress(
			TextureCategory previousCategory,
			GameAddress gameAddress) const override;

		virtual bool TryApplyFpsFix() override;

	private:
		uint32_t ReadU32(HANDLE module, uint32_t offset) const;
		void WriteU32(HANDLE module, uint32_t offset, uint32_t value);
		GameVersion GetGameVersion();
		void InitializeTextureHashPrefixTable();
		void FixBadRegistrySettings();

		bool _isPd2;
		HANDLE _hProcess;
		HANDLE _hGameExe;
		HANDLE _hD2ClientDll;
		GameVersion _version;
	};
}
