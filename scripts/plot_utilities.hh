#ifndef PLOT_UTILITIES_H
#define PLOT_UTILITIES_H

#include "TVirtualPad.h"
#include <TH1.h>
#include <TPaveStats.h>
#include <TString.h>

inline void move_statbox(TH1* hist, const std::string& location) {
    gPad->Update();
    TPaveStats* stats = (TPaveStats*)hist->FindObject("stats");
    if (!stats) return;

    stats->SetTextColor(hist->GetLineColor());
    double x1, y1, x2, y2;

    if (location == "upper right") {
        x1 = 0.65; x2 = 0.88; y1 = 0.75; y2 = 0.88;
    } else if (location == "upper left") {
        x1 = 0.15; x2 = 0.38; y1 = 0.75; y2 = 0.88;
    } else if (location == "lower right") {
        x1 = 0.65; x2 = 0.88; y1 = 0.15; y2 = 0.28;
    } else if (location == "lower left") {
        x1 = 0.15; x2 = 0.38; y1 = 0.15; y2 = 0.28;
    } else if (location == "upper center") {
        x1 = 0.38; x2 = 0.62; y1 = 0.75; y2 = 0.88;
    } else if (location == "lower center") {
        x1 = 0.38; x2 = 0.62; y1 = 0.15; y2 = 0.28;
    } else {
        return;
    }

    stats->SetX1NDC(x1);
    stats->SetX2NDC(x2);
    stats->SetY1NDC(y1);
    stats->SetY2NDC(y2);
    gPad->Modified();
}
#endif // PLOT_UTILITIES_H
