#pragma once

#include "LineDrawer.hpp"
#include <TCanvas.h>
#include <TH1I.h>
#include <TRandom3.h>
#include <fmt/core.h>

extern const unsigned int SEED_NUM;

void Print(std::string_view str);
struct MeanError
{
    float mean = 0.;
    float err = 0.;
};

struct Result
{
    MeanError stat{};
    float mid_prob = 0.;
    float pre_prob = 0.;
    float post_prob = 0.;
};

template <typename T>
auto GetCenterBoundary(const std::vector<T>& vec, double multiplier = 1)
{
    if (vec.size() < 3)
    {
        throw std::logic_error("vector size too small!");
    }
    const auto offSet = (vec.end() - vec.begin()) / 2;
    auto start = accumulate(vec.begin(), vec.begin() + offSet, 0.);
    auto end = start + vec.at(offSet);
    return std::make_pair(start * multiplier, end * multiplier);
}

class UniformInserter
{
  public:
    ~UniformInserter() = default;
    UniformInserter(const UniformInserter&) = delete;
    UniformInserter(UniformInserter&&) = default;
    auto operator=(const UniformInserter&) -> UniformInserter& = delete;
    auto operator=(UniformInserter&&) -> UniformInserter& = default;

    explicit UniformInserter(unsigned int num, std::string_view histname)
    {
        TH1::AddDirectory(false);
        constexpr int hist_entries = 1500;
        // Print(fmt::format("creating histogram with name {}", histname));
        histogram_ = std::make_unique<TH1I>(histname.data(), histname.data(), hist_entries, 0, num);
    }

    template <typename T>
    void operator()(const std::vector<T>& vec)
    {
        const auto [start, end] = GetCenterBoundary(vec);
        auto rndValue = rnd.Uniform(start, end);
        histogram_->Fill(rndValue);
    }

    [[nodiscard]] auto GetHist() -> auto*
    {
        return histogram_.get();
    }

    void Init()
    {
        result_ = {};
    }

    [[nodiscard]] auto Finish()
    {
        SetResult();
        histogram_->Reset("M");
        return result_;
    }

    void Draw(std::pair<double, double> boundary, std::string_view filename = "distri.png")
    {
        constexpr int default_canvas_width = 1000;
        constexpr int default_canvas_height = 800;
        auto canvas = TCanvas{ "canvas", "canvas", default_canvas_width, default_canvas_height };
        histogram_->Draw();
        canvas.Update();

        auto linesDraw = LineDrawer(2);
        const auto& [start, end] = boundary;
        auto lineL = linesDraw.Draw_vLine(&canvas, start);
        auto lineR = linesDraw.Draw_vLine(&canvas, end);
        lineL->Draw();
        lineR->Draw();

        canvas.SaveAs(filename.data());
    }

  private:
    std::unique_ptr<TH1I> histogram_;
    TRandom3 rnd = TRandom3{ SEED_NUM };
    MeanError result_;

    void SetResult()
    {
        auto mean = static_cast<float>(histogram_->GetMean());
        auto err = static_cast<float>(histogram_->GetStdDevError());
        result_ = MeanError{ mean, err };
    }
};
