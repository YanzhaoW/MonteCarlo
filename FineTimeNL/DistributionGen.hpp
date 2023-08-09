#pragma once
#include <Math/GSLRndmEngines.h>
#include <algorithm>
#include <ranges>

template <int value>
concept OddLargeNumber = (value % 2 != 0) and (value >= 3);

template <int size_>
    requires OddLargeNumber<size_>
class DisGenerator
{
  public:
    explicit DisGenerator()
    {
        engine_.Initialize();
        engine_.SetSeed(0);
    }
    auto Generate(std::vector<double>& distribution, double value) const
    {
        distribution.clear();
        distribution.reserve(size_);
        value = (value == 0.) ? engine_() : value;
        distribution.push_back(value);
        auto sum = 0.;
        for (size_t counter{}; counter < size_ - 2; ++counter)
        {
            sum += distribution.back();
            distribution.push_back(engine_() * (1 - sum));
        }
        sum += distribution.back();
        distribution.push_back(1 - sum);
        std::iter_swap(distribution.begin(), distribution.begin() + (size_ / 2));
    }
    auto operator()(std::vector<double>& distribution, double mid_prob = 0.) const
    {
        Generate(distribution, mid_prob);
        size_t mid_index = distribution.size() / 2;
        return std::make_tuple(distribution[mid_index - 1], distribution[mid_index], distribution[mid_index + 1]);
    }

  private:
    ROOT::Math::GSLRandomEngine engine_ = {};
};
