#include <Math/GSLRndmEngines.h>
#include <TCanvas.h>
#include <TH1I.h>
#include <TRandom3.h>
#include <iostream>
#include <numeric>
#include <string>

constexpr unsigned int BINSIZE = 5;

struct Result
{
    double mean = 0.;
    double error = 0.;
};

class MultiNomial
{
  public:
    MultiNomial() = default;
    explicit MultiNomial(unsigned int totalNum)
        : totalEntryN_{ totalNum }
    {
        engine_.Initialize();
        engine_.SetSeed(0);
    }
    void SetLoopSize(unsigned int size) { loopSize_ = size; }
    template <typename... Args>
    void SetHist(Args&&... args)
    {
        th1 = TH1I{ std::forward<Args>(args)... };
    }

    auto RandomFill(const std::vector<double>& distribution) { return engine_.Multinomial(totalEntryN_, distribution); }

    template <typename Operation>
    auto Loop_on(const std::vector<double>& distribution, Operation opt)
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
    ROOT::Math::GSLRandomEngine engine_ = {};
    TH1I th1 = TH1I{};
};

template <int size_>
class DisGenerator
{
  public:
    explicit DisGenerator()
    {
        static_assert(size_ % 2 != 0, "size must be odd!");
        engine_.Initialize();
        engine_.SetSeed(0);
    }
    auto Generate(double value, std::vector<double>& res) const
    {
        res.clear();
        res.reserve(size_);
        res.push_back(value);
        for (size_t i{}; i < size_ - 1; ++i)
        {
            auto sum = std::accumulate(res.begin(), res.end(), 0.);
            res.push_back(engine_() * (1 - sum));
        }
        std::iter_swap(res.begin(), res.begin() + (size_ / 2));
    }
    auto operator()(double mid_prob, std::vector<double>& res) const { Generate(mid_prob, res); }

  private:
    ROOT::Math::GSLRandomEngine engine_ = {};
};

auto main() -> int
{
    const auto totalEntry = 10000;
    const auto loopSize = 10000;

    auto generator = DisGenerator<BINSIZE>{};
    auto distri = std::vector<double>{};

    auto multiNml = MultiNomial{ totalEntry };

    generator(0.3, distri);
    for (const auto& value : distri)
    {
        std::cout << value << "\n";
    }

    constexpr int hist_entries = 1500;
    TH1I th1 = TH1I{ "hist", "hist", hist_entries, 0, 0 + totalEntry };
    auto rnd = TRandom3{ 0 };

    auto loopOp = [&th1, &rnd](const auto& vec)
    {
        auto offSet = vec.size() / 2;
        auto start = std::accumulate(vec.begin(), vec.begin() + offSet, 0.);
        auto end = start + vec.at(offSet);
        auto rndValue = rnd.Uniform(start, end);
        th1.Fill(rndValue);
    };

    multiNml.SetLoopSize(loopSize);
    multiNml.Loop_on(distri, loopOp);

    // for(size_t count{}; count < 10; ++count)
    // {
    //     auto vec = std::vector<double>{};
    //     generator(0.3, vec);
    //     for(const auto& value : vec)
    //     {
    //         std::cout << value << "\n";
    //     }
    //     std::cout << "-----------\n";
    // }

    // for ()

    auto canvas = TCanvas{};
    th1.Draw();
    canvas.SaveAs("distri.png");

    std::cout << "mean: " << th1.GetMean() << "\n";

    return 0;
}
