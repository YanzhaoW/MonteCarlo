#pragma once
#include <TCanvas.h>
#include <TLine.h>

class LineDrawer
{
  public:
    LineDrawer() = default;
    explicit LineDrawer(Width_t width)
        : width_{ width }
    {
    }

    auto Draw_vLine(TCanvas* canvas, double x_value) const
    {
        auto ymin = canvas->GetUymin();
        auto ymax = canvas->GetUymax();
        // std::cout << "ymin: " << ymin << "\n";
        // std::cout << "ymax: " << ymax << "\n";
        auto line = std::make_unique<TLine>(x_value, ymin, x_value, ymax);
        line->SetVertical(true);
        line->SetLineColor(color_);
        line->SetLineWidth(width_);
        return line;
    }

  private:
    Width_t width_ = {};
    Color_t color_ = kRed;
};
