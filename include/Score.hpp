#pragma once

namespace CCTV {
	class IScoreable {
	public:
		virtual double GetScore() = 0;
	};
};