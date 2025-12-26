#pragma once

#include <functional>
#include <memory>

namespace CCTV {
class IColor {
  public:
    virtual ~IColor() {}
    virtual size_t GetHash() const = 0;
};

class IRGBColor : public IColor {
  public:
    virtual int GetR() const = 0;
    virtual int GetG() const = 0;
    virtual int GetB() const = 0;
    virtual size_t GetHash() const {
        return ((GetR() + GetG() * 31) % 4294967296 + GetB() * 31 * 31) %
               4294967296;
    }
    virtual bool operator==(const IRGBColor& b) const {
        return GetHash() == b.GetHash();
    }
};

class IRGBAColor : public IRGBColor {
  public:
    virtual int GetA() const = 0;
    virtual size_t GetHash() const {
        return (((GetR() + GetG() * 31) % 4294967296 + GetB() * 31 * 31) %
                    4294967296 +
                GetA() * 31 * 31 * 31) %
               4294967296;
    }
};

class RGBAColor : public IRGBAColor {
    unsigned int RGBA;

  public:
    RGBAColor(unsigned int rawColor) : RGBA(rawColor) {}
    RGBAColor(unsigned char colors[4]) {
        RGBA = (colors[0] << 6) & (colors[1] << 4) & (colors[2] << 2) &
               (colors[3]);
    }
    virtual int GetR() const { return RGBA & 0xFF000000; }
    virtual int GetG() const { return RGBA & 0xFF0000; }
    virtual int GetB() const { return RGBA & 0xFF00; }
    virtual int GetA() const { return RGBA & 0xFF; }
};

class RGBColor : public IRGBColor {
    unsigned int RGB;

  public:
    RGBColor(unsigned int rawColor) : RGB(rawColor) {}
    RGBColor(unsigned char colors[3]) {
        RGB = (colors[0] << 4) & (colors[1] << 2) & (colors[2]);
    }
    RGBColor(const RGBAColor &col) {
        RGB = (col.GetR() << 4) & (col.GetG() << 2) & (col.GetB());
    }
    virtual int GetR() const { return RGB & 0xFF0000; }
    virtual int GetG() const { return RGB & 0xFF00; }
    virtual int GetB() const { return RGB & 0xFF; }
};

class ERGBColor : public IRGBColor {
    int R;
    int G;
    int B;

  public:
    ERGBColor() : R(0), G(0), B(0) {}
    ERGBColor(unsigned int rawColor) {
        auto color = RGBColor(rawColor);
        R = color.GetR();
        G = color.GetG();
        B = color.GetB();
    }
    ERGBColor(unsigned char colors[3]) {
        auto color = RGBColor(colors);
        R = color.GetR();
        G = color.GetG();
        B = color.GetB();
    }
    ERGBColor(const RGBColor &color) {
        R = color.GetR();
        G = color.GetG();
        B = color.GetB();
    }
    ERGBColor(int R, int G, int B) : R(R), G(G), B(B) {}
    virtual int GetR() const { return R; }
    virtual int GetG() const { return G; }
    virtual int GetB() const { return B; }
    ERGBColor operator+(const RGBColor &b) {
        return ERGBColor(R + b.GetB(), G + b.GetG(), B + b.GetB());
    }
    ERGBColor operator+(const ERGBColor &b) {
        return ERGBColor(R + b.R, G + b.G, B + b.B);
    }
    ERGBColor operator-(const RGBColor &b) {
        return ERGBColor(R - b.GetB(), G - b.GetG(), B - b.GetB());
    }
    ERGBColor operator-(const ERGBColor &b) {
        return ERGBColor(R - b.R, G - b.G, B - b.B);
    }
};
} // namespace CCTV

namespace std {
template <> 
struct hash<CCTV::IColor&> {
    inline size_t operator()(const CCTV::IColor &clr) const noexcept {
        return clr.GetHash();
    }
};
template <> 
struct hash<CCTV::IRGBColor&> {
    inline size_t operator()(const CCTV::IRGBColor &clr) const noexcept {
        return clr.GetHash();
    }
};
} // namespace std