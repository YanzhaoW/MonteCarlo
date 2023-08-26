#pragma once

#include <concepts>
#include <future>
#include <iostream>

class TH1;
template <int size1, std::size_t... sizes>
concept Equal = ((size1 == sizes) && ...);

template <int size1, std::size_t... sizes>
concept LessThan = ((size1 < sizes) && ...);

template <int size1, std::size_t... sizes>
concept NoLessThan = ((size1 >= sizes) && ...);

template <typename Type, typename Par>
concept Constructible_from = requires(Par par) { std::remove_cvref_t<Type>{ par }; };

static std::mutex PRINT_MUTEX; // NOLINT
inline void Print(std::string_view str)
{
    auto lock = std::scoped_lock{ PRINT_MUTEX };
    std::cout << str << "\n";
}

struct MeanError
{
    float mean = 0.;
    float err = 0.;
};

struct Parallel_run_input
{
    double pb = 0.1;
    double pbMax = 1.;
    double pa_begin = 0.;
    double pa_num = 0.;
    double pa_step = 0.;
    unsigned int entryN = 100;
    unsigned int rndNum = 1000;
    unsigned int entryNloopSize = 20;
};

struct Parallel_run_output
{
    unsigned int entryN;
    MeanError stat{};
    float pre_prob = 0.;
    float mid_prob = 0.;
    float post_prob = 0.;
    TH1* histogram;
};
