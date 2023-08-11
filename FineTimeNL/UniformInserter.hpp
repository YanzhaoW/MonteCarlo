#pragma once

#include "LineDrawer.hpp"
#include <Math/GSLRndmEngines.h>
#include <TCanvas.h>
#include <TH1I.h>
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

    explicit UniformInserter(ROOT::Math::GSLRandomEngine* engine, unsigned int num, std::string_view histname)
        : engine_{ engine }
    {
        TH1::AddDirectory(false);
        constexpr int hist_entries = 10000;
        constexpr int histBin_entries = 100;
        // Print(fmt::format("creating histogram with name {}", histname));
        histogram_ = std::make_unique<TH1I>(histname.data(), histname.data(), hist_entries, 0, num);
        // auto histBinName = fmt::format("{}_bin", histname);
        // histogramBin_ = std::make_unique<TH1I>(histBinName.c_str(), histBinName.c_str(), histBin_entries, 0, 10.);
        // auto histStartName = fmt::format("{}_start", histname);
        // histogramStart_ = std::make_unique<TH1I>(histStartName.c_str(), histStartName.c_str(), hist_entries, 0, num);
    }

    template <typename T>
    void operator()(const std::vector<T>& vec)
    {
        const auto [start, end] = GetCenterBoundary(vec);
        // histogramStart_->Fill(start);
        if (start == end)
        {
            return;
        }
        auto binValue = engine_->Rndm() * static_cast<double>(end - start);
        auto value = binValue + static_cast<double>(start);
        histogram_->Fill(value);
        // histogramBin_->Fill(binValue);
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

    void DrawAll(std::pair<double, double> boundary, std::string_view filename = "distri")
    {
        Draw(boundary, fmt::format("{}.png", filename));
        // Draw(fmt::format("{}_bin.png", filename), histogramBin_.get());
        // Draw(fmt::format("{}_start.png", filename), histogramStart_.get());
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

    auto GetEngine() -> auto*
    {
        return engine_;
    }

  private:
    std::unique_ptr<TH1I> histogram_;
    // std::unique_ptr<TH1I> histogramBin_;
    // std::unique_ptr<TH1I> histogramStart_;
    ROOT::Math::GSLRandomEngine* engine_ = nullptr;
    MeanError result_;

    void SetResult()
    {
        auto mean = static_cast<float>(histogram_->GetMean());
        auto err = static_cast<float>(histogram_->GetStdDevError());
        result_ = MeanError{ mean, err };
    }

    static void Draw(std::string_view filename, TH1* hist)
    {
        constexpr int default_canvas_width = 1000;
        constexpr int default_canvas_height = 800;
        auto canvas = TCanvas{ "canvas", "canvas", default_canvas_width, default_canvas_height };
        hist->Draw();
        canvas.Update();
        canvas.SaveAs(filename.data());
    }
};
