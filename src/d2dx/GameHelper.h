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

#include "Types.h"

namespace d2dx 
{
	class GameHelper final
	{
	public:
		GameHelper();
		~GameHelper();

		GameVersion GetVersion() const;
		const char* GetVersionString() const;

		uint32_t ScreenOpenMode() const;
		void GetConfiguredGameSize(int32_t* width, int32_t* height) const;
		void SetIngameMousePos(int32_t x, int32_t y);

		GameAddress IdentifyGameAddress(uint32_t returnAddress) const;

		TextureCategory GetTextureCategoryFromHash(uint32_t textureHash) const;
		TextureCategory RefineTextureCategoryFromGameAddress(TextureCategory previousCategory, GameAddress gameAddress) const;

	private:
		uint32_t ReadU32(HANDLE module, uint32_t offset) const;
		void WriteU32(HANDLE module, uint32_t offset, uint32_t value);
		GameVersion GetGameVersion();
		void InitializeTextureHashPrefixTable();

		HANDLE _hProcess;
		HANDLE _hGameExe;
		HANDLE _hD2ClientDll;
		GameVersion _version;
		bool _isPd2;
	};
}
