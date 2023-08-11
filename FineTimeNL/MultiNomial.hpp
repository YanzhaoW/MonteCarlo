#pragma once

#include <Math/GSLRndmEngines.h>
#include <TH1I.h>

extern const unsigned int SEED_NUM;

class MultiNomial
{
  public:
    explicit MultiNomial(ROOT::Math::GSLRandomEngine* engine)
        : engine_{ engine }
    {
    }
    void SetLoopSize(unsigned int size) { loopSize_ = size; }
    void SetEntrySize(unsigned int entryNum) { totalEntryN_ = entryNum; }
    void SetHist(auto&&... args) { th1 = TH1I{ std::forward<decltype(args)>(args)... }; }

    [[nodiscard]] auto RandomFill(const std::vector<double>& distribution) const
    {
        return engine_->Multinomial(totalEntryN_, distribution);
    }

    auto Loop_on(const std::vector<double>& distribution, std::invocable<decltype(distribution)> auto&& opt) const
    {
        for (size_t i{}; i < loopSize_; ++i)
        {
            auto entries = RandomFill(distribution);
            opt(entries);
        }
    }

  private:
    unsigned int totalEntryN_ = 0;
    unsigned int loopSize_ = 0;
    ROOT::Math::GSLRandomEngine* engine_ = nullptr;
    TH1I th1 = TH1I{};
};
