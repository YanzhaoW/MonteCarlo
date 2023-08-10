#pragma once
#include <Math/GSLRndmEngines.h>
#include <iostream>
#include <range/v3/algorithm.hpp>
#include <range/v3/view.hpp>

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

    template <int size>
    auto UniformSplit(double totalProb, auto&& back_inserter) const
    {
        assert(size > 1);
        auto vec = std::vector<double>(size, 0.);
        ranges::for_each(vec | ranges::views::drop_last(1),
                         [this, totalProb](auto& element) { element = engine_() * totalProb; });
        vec.back() = totalProb;
        ranges::sort(vec);
        std::accumulate(vec.begin(),
                        vec.end(),
                        0.,
                        [&back_inserter](double init, auto& element)
                        {
                            *back_inserter = element - init;
                            ++back_inserter;
                            return element;
                        });
        return vec;
    }

    auto Generate(std::vector<double>& distribution, double value, double max = 1.) const
    {
        distribution.clear();
        distribution.reserve(size_);
        value = (value == 0.) ? engine_() * max : value;
        distribution.push_back(value);
        UniformSplit<size_ - 1>(1 - value, std::back_inserter(distribution));
        // auto sum = 0.;
        // for (size_t counter{}; counter < size_ - 2; ++counter)
        // {
        //     sum += distribution.back();
        //     distribution.push_back(engine_() * (1 - sum));
        // }
        // sum += distribution.back();
        // distribution.push_back(1 - sum);
        std::iter_swap(distribution.begin(), distribution.begin() + (size_ / 2));
    }

    auto operator()(std::vector<double>& distribution, double mid_prob = 0., double max = 1.) const
    {
        Generate(distribution, mid_prob, max);
        size_t mid_index = distribution.size() / 2;
        return std::make_tuple(distribution[mid_index - 1], distribution[mid_index], distribution[mid_index + 1]);
    }

  private:
    ROOT::Math::GSLRandomEngine engine_ = {};
};
