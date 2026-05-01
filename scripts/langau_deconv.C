#include "TF1.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TVirtualPad.h"

void langau_deconv() {
    gStyle->SetOptStat(0);

    double xmin = -5, xmax = 30;

    // -------------------------------
    // 1. Gaussian
    TF1* gaus = new TF1("gaus", "gaus", xmin, xmax);
    double gaus_amp = 1.0, gaus_mean = 5.0, gaus_sigma = 2.0;
    gaus->SetParameters(gaus_amp, gaus_mean, gaus_sigma);

    // Normalize Gaussian
    double gaus_area = gaus->Integral(xmin, xmax);
    gaus->SetParameter(0, gaus_amp / gaus_area);

    gaus->SetLineColor(kBlue);
    gaus->SetLineWidth(2);
    gaus->SetTitle("Normalized Gaussian, Landau, and Langau;x;Probability Density");

    // -------------------------------
    // 2. Landau
    TF1* landau = new TF1("landau", "landau", xmin, xmax);
    double landau_amp = 1.0, landau_mpv = 5.0, landau_width = 2.0;
    landau->SetParameters(landau_amp, landau_mpv, landau_width);

    // Normalize Landau
    double landau_area = landau->Integral(xmin, xmax);
    landau->SetParameter(0, landau_amp / landau_area);

    landau->SetLineColor(kRed);
    landau->SetLineWidth(2);

    // -------------------------------
    // 3. Langau (Landau ⊗ Gaussian)
    auto langaufun = [](double* x, double* par) {
        double invsq2pi = 0.3989422804014;
        double mpshift = -0.22278298;
        double np = 100.0;
        double sc = 5.0;

        double mpc = par[1] - mpshift * par[0];
        double xlow = x[0] - sc * par[3];
        double xupp = x[0] + sc * par[3];
        double step = (xupp - xlow) / np;
        double sum = 0.0;
        double xx, fland;

        for (int i = 1; i <= np / 2; ++i) {
            xx = xlow + (i - 0.5) * step;
            fland = TMath::Landau(xx, mpc, par[0]) / par[0];
            sum += fland * TMath::Gaus(x[0], xx, par[3]);

            xx = xupp - (i - 0.5) * step;
            fland = TMath::Landau(xx, mpc, par[0]) / par[0];
            sum += fland * TMath::Gaus(x[0], xx, par[3]);
        }

        return (par[2] * step * sum * invsq2pi / par[3]);
    };

    TF1* langau = new TF1("langau", langaufun, xmin, xmax, 4);
    double langau_width = 2.0, langau_mpv = 5.0, langau_area_init = 1.0, langau_sigma = 1.5;
    langau->SetParameters(langau_width, langau_mpv, langau_area_init, langau_sigma);

    // Normalize Langau numerically
    double langau_area_val = langau->Integral(xmin, xmax);
    langau->SetParameter(2, langau_area_init / langau_area_val);

    langau->SetLineColor(kGreen + 2);
    langau->SetLineWidth(2);

    // -------------------------------
    // Plotting
    TCanvas* c1 = new TCanvas("c1", "Comparison", 900, 500);
    gPad->SetGrid();
    gaus->Draw();
    landau->Draw("SAME");
    langau->Draw("SAME");

    // -------------------------------
    // Legend
    TLegend* leg = new TLegend(0.45, 0.50, 0.89, 0.89);
    leg->SetHeader("Function Parameters", "C");
    leg->AddEntry(gaus, Form("Gaussian: #mu = %.1f, #sigma = %.1f", gaus_mean, gaus_sigma), "l");
    leg->AddEntry(landau, Form("Landau: MPV = %.1f, width = %.1f", landau_mpv, landau_width), "l");
    leg->AddEntry(langau, Form("Langau: MPV = %.1f, width = %.1f, #sigma = %.1f", langau_mpv, langau_width, langau_sigma), "l");

    leg->Draw();
    c1->SaveAs("./data/plots/langau_deconv.pdf");
}
