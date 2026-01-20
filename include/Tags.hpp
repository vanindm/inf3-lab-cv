#pragma once

#include <memory>
#include <string>

namespace CCTV {
class ITag {
  public:
    virtual std::string GetName() = 0;
    virtual void *GetParent() = 0;
};

class ITagged {
  public:
    virtual std::shared_ptr<ITag> GetTag() = 0;
    virtual void SetTag(std::shared_ptr<ITag> tag) = 0;
};

class HighScoreTag : public ITag {
    void *parent;

  public:
    HighScoreTag(void *parent) : parent(parent) {}
    virtual ~HighScoreTag() {}
    virtual void *GetParent() { return parent; }
    virtual std::string GetName() { return "Высокая значимость"; }
};

class ScoreLeapTag : public ITag {
    void *parent;

  public:
    ScoreLeapTag(void *parent) : parent(parent) {}
    virtual ~ScoreLeapTag() {}
    virtual void *GetParent() { return parent; }
    virtual std::string GetName() { return "Скачок значимости"; }
};

class FlashTag : public ITag {
    void *parent;

  public:
    FlashTag(void *parent) : parent(parent) {}
    virtual ~FlashTag() {}
    virtual void *GetParent() { return parent; }
    virtual std::string GetName() { return "Вспышка"; }
};
}; // namespace CCTV