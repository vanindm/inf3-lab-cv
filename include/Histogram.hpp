#pragma once

namespace CCTV {
	template<class T, class U>
	class IHistogram {
	public:
		U GetFrequency(T bin) const;
	};
}