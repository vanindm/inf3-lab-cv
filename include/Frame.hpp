#pragma once

#include <memory>

#include "Colorspaces.hpp"
#include "Score.hpp"
#include "Histogram.hpp"
#include <PATypes/PairTuple.h>
#include <PATypes/Sequence.h>
#include <PATypes/HashMap.h>

#define STB_IMAGE_IMPLEMENTATION
#include "contrib/stb_image.h"

namespace CCTV {
struct Dot {
	size_t x, y;
};

class IFrame {
  public:
	virtual std::shared_ptr<IRGBColor> GetPoint(const Dot &at) const = 0;
	virtual ~IFrame() {};
};

class IIFrame {
  public:
	virtual std::shared_ptr<PATypes::Sequence<IFrame>>
	GetSubFrame(Dot upperLeftCorner, Dot bottomRightCorner) const;
	virtual ~IIFrame() = 0;
};

class Frame : public IFrame {
	unsigned char *data;
	int width, height, channels;
	Frame() : width(0), height(0), channels(0) {}
	class FrameHistogram : IHistogram<IRGBColor, int> {
		PATypes::HashMap<IRGBColor&, int> storage;
	public:
		FrameHistogram(const Frame& frame) {
			for (size_t i = 0; i < frame.width; ++i) {
				for (size_t j = 0; j < frame.height; ++j) {
					try {
						storage.Add(*frame.GetPoint({i, j}), (storage.Get(*frame.GetPoint({i, j}))));
					} catch (std::out_of_range& ) {
						storage.Add(*frame.GetPoint({i, j}), 1);
					}
				}
			}
		}
		virtual int GetFrequency(IRGBColor& color) const {
			try {
				return storage.Get(color);
			} catch (std::out_of_range&) {
				return 0;
			}
		}
	};
  public:
	Frame(const Frame &frame)
		: width(frame.width), height(frame.height), channels(frame.channels) {
		this->data =
			(unsigned char *)malloc(frame.channels * frame.width * frame.height);
		for (int i = 0; i < frame.channels * frame.width * frame.height; ++i) {
			this->data[i] = frame.data[i];
		}
	}
	Frame(Frame &&frame)
		: width(frame.width), height(frame.height), channels(frame.channels) {
		data = frame.data;
		frame.data = nullptr;
	}
	virtual ~Frame() { stbi_image_free(data); }
	static std::shared_ptr<Frame> FromFile(const std::string &filename) {
		std::shared_ptr<Frame> newFrame = std::make_shared<Frame>(Frame());
		newFrame->data = stbi_load(filename.c_str(), &newFrame->width,
								  &newFrame->height, &newFrame->channels, 3);
		newFrame->channels = 3;
		if (!newFrame->data) {
			throw std::runtime_error(stbi_failure_reason());
		}
		return newFrame;
	}
	std::shared_ptr<IRGBColor> GetPoint(const Dot &at) const {
		std::shared_ptr<RGBColor> res =
			std::make_shared<RGBColor>(data[(at.x + at.y * width) * channels]);
		return res;
	}
	Frame delta(const Frame &b) const {
		Frame newFrame(*this);
		if (width != b.width || height != b.height || channels != b.channels)
			throw std::logic_error("кадры несовместимы для операции delta");
		for (int i = 0; i < width * height * channels; ++i) {
			newFrame.data[i] = std::abs((int)data[i] - b.data[i]);
		}
		return newFrame;
	}
	Frame XOR(const Frame &b) const {
		Frame newFrame(*this);
		if (width != b.width || height != b.height || channels != b.channels)
			throw std::logic_error("кадры несовместимы для операции XOR");
		for (int i = 0; i < width * height * channels; ++i) {
			newFrame.data[i] = data[i] ^ b.data[i];
		}
		return newFrame;
	}
	Frame AND(const Frame &b) const {
		Frame newFrame(*this);
		if (width != b.width || height != b.height || channels != b.channels)
			throw std::logic_error("кадры несовместимы для операции XOR");
		for (int i = 0; i < width * height * channels; ++i) {
			newFrame.data[i] = (data[i] & b.data[i]);
		}
		return newFrame;
	}
	Frame Map(IRGBColor& (*f)(const IRGBColor& a)) const {
		Frame newFrame(*this);
		for (int i = 0; i < width * height; i += channels) {
			IRGBColor& res = f(RGBColor(newFrame.data + i));
			newFrame.data[i] = res.GetR();
			newFrame.data[i + 1] = res.GetG();
			newFrame.data[i + 2] = res.GetB();
		}
		return newFrame;
	}
	template<class T>
	T Reduce(T (*f)(const T&, const IRGBColor&), IRGBColor& init) const {
		T result = f(T(0), init);
		for (int i = 0; i < width * height; i += channels) {
			result = f(result, RGBColor(data + i));
		}
		return result;
	}
	double norm() const {
		double res = 0;
		for (int i = 0; i < width * height * channels; ++i) {
			res += data[i];
		}
		res /= (width * height);
		return res;
	}
	Frame &operator=(const Frame &other) {
		if (this == &other)
			return *this;
		stbi_image_free(data);
		this->data =
			(unsigned char*) malloc(other.channels * other.width * other.height);
		for (int i = 0; i < other.channels * other.width * other.height; ++i) {
			this->data[i] = other.data[i];
		}
		this->width = other.width;
		this->height = other.height;
		this->channels = other.channels;
		return *this;
	}
};

class FrameSequence : public PATypes::MutableListSequence<Frame>,
					  public IScoreable {
	int windowLength;
	double GetDeltaScore() {
		int lastIndex = getLength() - 1;
		if (lastIndex < 0 || windowLength < 2) {
			return 0;
		}
		Frame result = get(lastIndex);
		for (int i = 1; i < windowLength; ++i) {
			result = result.AND(get(lastIndex - i));
		}
		return get(lastIndex).delta(result).norm();
	}
	double GetDeltaScore2() {
		int lastIndex = getLength() - 1;
		if (lastIndex < 0 || windowLength < 2) {
			return 0;
		}
		double result = 0;
		Frame lastFrame = get(lastIndex);
		for (int i = 1; i < windowLength; ++i) {
			result += lastFrame.delta(get(lastIndex - i)).norm();
			lastFrame = get(lastIndex - i);
		}
		return result;

	}
  public:
	FrameSequence(Frame *items, int count, int windowLength)
		: PATypes::MutableListSequence<Frame>(items, count),
		  windowLength(windowLength) {}
	FrameSequence(PATypes::Sequence<Frame> &sequence, int windowLength)
		: PATypes::MutableListSequence<Frame>(sequence),
		  windowLength(windowLength) {}
	static FrameSequence Where(bool (*f)(const Frame&), FrameSequence& input, int windowLength = -1) {
		FrameSequence result(0);
		if (windowLength == -1)
			result = FrameSequence(input.windowLength);
		else
			result = FrameSequence(windowLength);
		auto enumerator = input.getEnumerator();
		while (enumerator->moveNext()) {
			if (f(enumerator->current())) {
				result.append(enumerator->current());
			}
		}
		return result;
	}
	static FrameSequence Where(bool (*f)(const Frame&), FrameSequence& sequence) {
		FrameSequence seq(0);
		auto enumerator = sequence.getEnumerator();
		while (enumerator->moveNext()) {
			if (f(enumerator->current())) {
				seq.append(enumerator->current());
			}
		}
		return seq;
	}
	FrameSequence(int windowLength)
		: PATypes::MutableListSequence<Frame>(), windowLength(windowLength) {}
	FrameSequence(Frame item, int windowLength)
		: PATypes::MutableListSequence<Frame>(), windowLength(windowLength) {}
	virtual FrameSequence& Map(Frame (*f)(const Frame&)) {
		auto enumerator = getEnumerator();
		while (enumerator->moveNext()) {
			enumerator->current() = f(enumerator->current());
		}
		return *this;
	}
	virtual double GetScore() {
		return GetDeltaScore2() * 1.0;
	}
};
} // namespace CCTV