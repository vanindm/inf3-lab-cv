#pragma once

#include <PATypes/Sequence.h>

#include "Colorspaces.hpp"
#include "Frame.hpp"

namespace CCTV {

struct Rect {
    int x, y, width, height;

    Rect() : x(0), y(0), width(0), height(0) {}
    Rect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}
};

struct Feature {
    struct Rectangle {
        Rect rect;
        float weight;
    };

    PATypes::Sequence<Rectangle> *rectangles;
    int featureType; // 2-rect (type 0), 3-rect (type 1), 4-rect (type 2)
};

struct Classifier {
    Feature feature;
    float threshold;
    float leftValue;
    float rightValue;
    int leftNode;
    int rightNode;
};

struct Stage {
    PATypes::Sequence<Classifier> *classifiers;
    float threshold;
};

struct CascadeData {
    PATypes::Sequence<Stage> *stages;
    int windowWidth;
    int windowHeight;
};

class IImage : IIFrame {
  private:
    PATypes::MutableArraySequence<PATypes::MutableArraySequence<ERGBColor>> data;
    int width, height;

  public:
    IImage() : width(0), height(0) {}

    void compute(PATypes::Sequence<PATypes::Sequence<RGBAColor> *> *image) {
        height = image->getLength();
        width = image->getFirst()->getLength();
        data = PATypes::MutableArraySequence<PATypes::MutableArraySequence<ERGBColor>>(height + 1);

        for (int i = 0; i < height + 1; ++i) {
            data.Getrvalue(i) = PATypes::MutableArraySequence<ERGBColor>(width + 1);
            for (int j = 0; j < width + 1; ++j) {
                data.Getrvalue(i).Getrvalue(j) = (unsigned int) 0;
            }
        }

        for (int y = 1; y <= height; ++y) {
            for (int x = 1; x <= width; ++x) {
                data.Getrvalue(y).Getrvalue(x) = ERGBColor(RGBColor(image->get(y)->get(x - 1))) + data.get(y).get(x) +
                             data.get(y).get(x) - data.get(y - 1).get(x - 1);
            }
        }
    }

    ERGBColor getRectSum(const Rect &rect) {
        int x1 = rect.x;
        int y1 = rect.y;
        int x2 = rect.x + rect.width;
        int y2 = rect.y + rect.height;

        if (x1 < 0)
            x1 = 0;
        if (y1 < 0)
            y1 = 0;
        if (x2 > width)
            x2 = width;
        if (y2 > height)
            y2 = height;

        if (x1 >= x2 || y1 >= y2)
            return (unsigned int) 0;

        return data.get(y2).get(x2) - data.get(y1).get(x2) + data.get(y1).get(x1);
    }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
};

} // namespace CCTV