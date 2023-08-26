#pragma once
#include <Math/GSLRndmEngines.h>
#include <iostream>
#include <range/v3/all.hpp>
#include <span>

extern const unsigned int SEED_NUM;

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
        engine_.SetSeed(SEED_NUM);
    }

    auto UniformSplit(double totalProb, std::span<double> distribution) const
    {
        auto vec = std::vector<double>(distribution.size(), 0.);
        ranges::for_each(vec | ranges::views::drop_last(1),
                         [this, totalProb](auto& element) { element = engine_() * totalProb; });
        vec.back() = totalProb;
        ranges::sort(vec);
        auto init = 0.;
        ranges::transform(vec,
                          distribution.begin(),
                          [&init](double element)
                          {
                              auto old_init = init;
                              init = element;
                              return element - old_init;
                          });
    }

    auto Generate(std::span<double> distribution, double value, double max = 1.) const
    {
        ranges::for_each(distribution, [](double& element) { element = 0; });
        value = (value == 0.) ? engine_() * max : value;
        distribution.front() = value;
        UniformSplit(1 - value, distribution.subspan(1));
        std::iter_swap(distribution.begin(), distribution.begin() + (size_ / 2));
    }

    auto operator()(std::span<double> distribution, double mid_prob = 0., double max = 1.) const
    {
        Generate(distribution, mid_prob, max);
        size_t mid_index = distribution.size() / 2;
        return std::make_tuple(distribution[mid_index - 1], distribution[mid_index], distribution[mid_index + 1]);
    }

  private:
    ROOT::Math::GSLRandomEngine engine_ = {};
};
