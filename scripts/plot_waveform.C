#include <vector>
#include <string>
#include <iostream>

#include "TROOT.h"
#include "TString.h"
#include "TStyle.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TF1.h"
#include "TVirtualPad.h"
#include "TProfile.h"
#include "TKey.h"
#include "TFile.h"


using namespace std;
/*
* @brief: Plot the raw wavefrom from fADC
* @params: None
* @return: None
*/
string waveform_rootfile = "data/rootfiles/archive/Showermax_131445_waveform.root";
void plot_waveform() {
    gStyle->SetOptStat(0);

    std::unique_ptr<TFile> file(TFile::Open(waveform_rootfile.c_str(), "READ"));
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file " << waveform_rootfile << std::endl;
        return;
    }

    // Read and clone all TProfiles
    std::vector<TProfile*> profiles;
    TIter next(file->GetListOfKeys());
    TKey* key;
    while ((key = (TKey*)next())) {
        TClass* cl = gROOT->GetClass(key->GetClassName());
        if (!cl || !cl->InheritsFrom(TProfile::Class())) continue;

        TProfile* original = dynamic_cast<TProfile*>(key->ReadObj());
        if (original) {
            TProfile* clone = (TProfile*)original->Clone();
            clone->SetDirectory(nullptr);  // <-- IMPORTANT: Detach from file
            profiles.push_back(clone);
        }
    }

    file->Close();

    // Plot in 5x4 grid per canvas
    const int profiles_per_canvas = 20;
    const int columns = 5, rows = 4;
    int total = profiles.size();
    int canvas_count = (total + profiles_per_canvas - 1) / profiles_per_canvas;

    for (int c = 0; c < canvas_count; ++c) {
        TCanvas* canvas = new TCanvas(Form("canvas_%d", c+1), "Waveform Profiles", 1600, 1200);
        canvas->Divide(columns, rows);

        for (int i = 0; i < profiles_per_canvas; ++i) {
            int idx = c * profiles_per_canvas + i;
            if (idx >= total) break;

            canvas->cd(i+1);
            TProfile* prof = profiles[idx];
            prof->SetTitle(Form("Profile %d", idx + 1));
            prof->GetXaxis()->SetRangeUser(0, 100);
            prof->Draw("hist");
        }

        canvas->Update();
    }
}
