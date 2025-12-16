#pragma once

#include <memory>
#include <string>

namespace CCTV {
	class ITag {
		virtual std::string GetName() = 0;
	};

	template<typename T>
	class ITagged {
	public:
		virtual std::shared_ptr<ITag> GetTag() = 0;
	};
};