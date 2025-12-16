#pragma once

#include <memory>

#include <PATypes/PairTuple.h>
#include <PATypes/Sequence.h>
#include "Colorspaces.hpp"

struct Dot {
	size_t x, y;
};

namespace CCTV {
	class IFrame {
	public:
		virtual std::shared_ptr<IColor> GetPoint() = 0;
	};

	class IIFrame {
	public:
		virtual std::shared_ptr<PATypes::Sequence<IFrame>> GetSubFrame(Dot upperLeftCorner, Dot bottomRightCorner);
	};
}