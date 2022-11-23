#include "pch.h"
#include "D2Types.h"

namespace d2dx
{
	namespace D2
	{
		Offset StaticPath::GetPos() const noexcept {
			return {
				static_cast<int32_t>((xPos << 16) + xOffset),
				static_cast<int32_t>((yPos << 16) + yOffset),
			};
		}

		Offset Unit109::GetPos() const noexcept {
			switch (dwType) {
			case UnitType::Player:
			case UnitType::Monster:
			case UnitType::Missile:
				return path->GetPos();
			default:
				return staticPath->GetPos();
			}
		}

		Offset Unit110::GetPos() const noexcept {
			switch (dwType) {
			case UnitType::Player:
			case UnitType::Monster:
			case UnitType::Missile:
				return path->GetPos();
			default:
				return staticPath->GetPos();
			}
		}

		Offset Unit112::GetPos() const noexcept {
			switch (dwType) {
			case UnitType::Player:
			case UnitType::Monster:
			case UnitType::Missile:
				return path->GetPos();
			default:
				return staticPath->GetPos();
			}
		}
	}
}