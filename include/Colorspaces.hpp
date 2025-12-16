#pragma once

#include <memory>

namespace CCTV {
	class IColor {};

	class RGBAColor : IColor {
		unsigned int RGBA;
	public:
		RGBAColor(unsigned int rawColor) : RGBA(rawColor) {}
		RGBAColor(unsigned char colors[4]) {
			RGBA = (colors[0] << 4096) & (colors[1] << 512) & (colors[2] << 64) & (colors[3]);
		}
		int GetR() {
			return RGBA & 0xFF000000;
		}
		int GetB() {
			return RGBA & 0xFF0000;
		}
		int GetB() {
			return RGBA & 0xFF00;
		}
		int GetA() {
			return RGBA & 0xFF;
		}
	};
};