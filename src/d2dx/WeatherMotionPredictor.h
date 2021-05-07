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
#include "IRenderContext.h"

namespace d2dx
{
#pragma once
	class WeatherMotionPredictor
	{
	public:
		WeatherMotionPredictor(
			_In_ const std::shared_ptr<IGameHelper>& gameHelper);

		void Update(
			_In_ IRenderContext* renderContext);

		OffsetF GetOffset(
			_In_ int32_t particleIndex,
			_In_ OffsetF posFromGame);

	private:
		struct ParticleMotion final
		{
			uint32_t lastUsedFrame = 0;
			OffsetF lastPos = { 0, 0 };
			OffsetF velocity = { 0, 0 };
			OffsetF predictedPos = { 0, 0 };
			int64_t dtLastPosChange = 0;
		};

		std::shared_ptr<IGameHelper> _gameHelper;
		uint32_t _frame = 0;
		float _dt = 0;
		Buffer<ParticleMotion> _particleMotions;
	};
}
