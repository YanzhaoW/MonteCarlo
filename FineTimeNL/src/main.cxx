#include "FineTimeMC.hpp"
#include "Sinker.hpp"
#include <chrono>
#include <cxxopts.hpp>
#include <iostream>
#include <string>

const unsigned int SEED_NUM = 0;

enum class Mode
{
    none,
    pa,
    entryN,
    fix
};

Mode Str2Mode(std::string_view name)
{
    if (name == "pa")
    {
        return Mode::pa;
    }
    else if (name == "entryN")
    {
        return Mode::entryN;
    }
    else if (name == "fix")
    {
        return Mode::fix;
    }
    else if (name == "none")
    {
        return Mode::none;
    }
    throw std::logic_error(fmt::format("mode {} cannot be resolved!", name));
}

auto main(int argc, char** argv) -> int
{
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    cxxopts::Options options("FineTimeNL", "Command flags for FineTime NeuLAND");
    options.add_options()("t,thread", "thread numbers", cxxopts::value<int>()->default_value("1"))(
        "m, mode", "modes: pa, entryN, fix, none", cxxopts::value<std::string>()->default_value("pre"))(
        "e, entryN", "number of full entryN", cxxopts::value<int>()->default_value("400"))(
        "e_min", "number of min full entryN", cxxopts::value<int>()->default_value("10"))(
        "e_max", "number of max full entryN", cxxopts::value<int>()->default_value("20"))(
        "r, r_num", "number of random values", cxxopts::value<int>()->default_value("1000"))(
        "pb", "probability of the central bin in fix distribution", cxxopts::value<double>()->default_value("0.01"))(
        "pa", "probability of the previous bin in fix distribution", cxxopts::value<double>()->default_value("0."))(
        "pa_size", "probability of the previous bin in fix distribution", cxxopts::value<int>()->default_value("200"))(
        "h,help", "Print usage");

    auto optresult = options.parse(argc, argv);
    if (optresult.count("help"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    const auto prob_b = optresult["pb"].as<double>();
    const auto prob_a = optresult["pa"].as<double>();
    auto fineTimeMC = FineTimeMC{};

    fineTimeMC.SetEntryN(optresult["entryN"].as<int>());
    fineTimeMC.SetThreadsNum(optresult["thread"].as<int>());
    fineTimeMC.SetRndNumber(optresult["r_num"].as<int>());

    // ----------------------------------------------------------------
    auto writer_entryN = CSVWriter{ [](auto* self, const Parallel_run_output& result)
                                    { self->add_row(result.entryN, result.stat.mean, result.stat.err); },
                                    CSVColumn<unsigned int>{ "entryN" },
                                    CSVColumn<float>{ "mean" },
                                    CSVColumn<float>{ "stderr" } };
    writer_entryN.SetFileName("entryN.csv");

    // ----------------------------------------------------------------
    auto writer_pre = CSVWriter{ [](auto* self, const Parallel_run_output& result)
                                 { self->add_row(result.pre_prob, result.stat.mean, result.stat.err); },
                                 CSVColumn<double>{ "pa" },
                                 CSVColumn<float>{ "mean" },
                                 CSVColumn<float>{ "stderr" } };
    writer_pre.SetFileName("pa.csv");

    // ----------------------------------------------------------------
    auto drawer = HistDrawer{ "distri.png", [](auto* self, const auto& result) { self->Set(result); } };

    //-----------------------------------------------------------------
    switch (Str2Mode(optresult["mode"].as<std::string>()))
    {
        case Mode::pa:
        {
            fineTimeMC.RunFixedPbAllPa(prob_b, 0., 1., optresult["pa_size"].as<int>(), writer_pre);
            break;
        }
        case Mode::entryN:
        {
            fineTimeMC.RunFixedPbAllEntryN(
                prob_b, optresult["e_min"].as<int>(), optresult["e_max"].as<int>(), writer_entryN);
            break;
        }
        case Mode::fix:
        {
            fineTimeMC.RunWithAllFixed({ prob_a, prob_b, 1 - prob_a - prob_b }, drawer);
            break;
        }
        case Mode::none:
        {
            break;
        }
    }

    // ----------------------------------------------------------------
    fineTimeMC.Wait();
    fineTimeMC.Write();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Execution time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
              << "[ms]" << std::endl;
    return 0;
}
