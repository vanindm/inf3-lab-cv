#pragma once

namespace CCTV {
	class IScoreable {
	public:
		virtual double GetScore(const std::optional<int>& r) = 0;
	};
};