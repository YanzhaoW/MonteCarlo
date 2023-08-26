#pragma once

#include "traits.hpp"
#include <Math/GSLRndmEngines.h>
#include <TH1I.h>
#include <fmt/core.h>

extern const unsigned int SEED_NUM;

class MultiNomial
{
  public:
    explicit MultiNomial(ROOT::Math::GSLRandomEngine* engine)
        : engine_{ engine }
    {
    }
    void SetRndNum(unsigned int size)
    {
        rndNum_ = size;
    }
    void SetEntryN(unsigned int num)
    {
        entryN_ = num;
    }

    auto GetEntryN() const -> auto
    {
        return entryN_;
    }

    void SetHist(auto&&... args)
    {
        th1 = TH1I{ std::forward<decltype(args)>(args)... };
    }

    [[nodiscard]] auto RandomFill(const auto& distribution) const
    {
        auto vec = std::vector<double>(distribution.begin(), distribution.end());
        return engine_->Multinomial(entryN_, vec);
    }

    auto Loop_on(const auto& distribution, std::invocable<decltype(distribution)> auto&& opt) const
    {
        for (size_t i{}; i < rndNum_; ++i)
        {
            auto entries = RandomFill(distribution);
            opt(entries);
        }
    }

  private:
    unsigned int entryN_ = 0;
    unsigned int rndNum_ = 0;
    ROOT::Math::GSLRandomEngine* engine_ = nullptr;
    TH1I th1 = TH1I{};
};
