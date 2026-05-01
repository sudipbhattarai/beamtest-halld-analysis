#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include "TCanvas.h"
#include "TH1.h"
#include "TF1.h"
#include "TLatex.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TMath.h"

using namespace std;

double langaufun(double *x, double *par) {

    //Fit parameters:
    //par[0]=Width (scale) parameter of Landau density
    //par[1]=Most Probable (MP, location) parameter of Landau density
    //par[2]=Total area (integral -inf to inf, normalization constant)
    //par[3]=Width (sigma) of convoluted Gaussian function
    //
    //In the Landau distribution (represented by the CERNLIB approximation),
    //the maximum is located at x=-0.22278298 with the location parameter=0.
    //This shift is corrected within this function, so that the actual
    //maximum is identical to the MP parameter.

    // Numeric constants
    double invsq2pi = 0.3989422804014;   // (2 pi)^(-1/2)
    double mpshift  = -0.22278298;       // Landau maximum location

    // Control constants
    double np = 100.0;      // number of convolution steps
    double sc =   5.0;      // convolution extends to +-sc Gaussian sigmas

    // Variables
    double xx;
    double mpc;
    double fland;
    double sum = 0.0;
    double xlow,xupp;
    double step;
    double i;


    // MP shift correction
    mpc = par[1] - mpshift * par[0];

    // Range of convolution integral
    xlow = x[0] - sc * par[3];
    xupp = x[0] + sc * par[3];

    step = (xupp-xlow) / np;

    // Convolution integral of Landau and Gaussian by sum
    for(i=1.0; i<=np/2; i++) {
        xx = xlow + (i-.5) * step;
        fland = TMath::Landau(xx,mpc,par[0]) / par[0];
        sum += fland * TMath::Gaus(x[0],xx,par[3]);

        xx = xupp - (i-.5) * step;
        fland = TMath::Landau(xx,mpc,par[0]) / par[0];
        sum += fland * TMath::Gaus(x[0],xx,par[3]);
    }

    return (par[2] * step * sum * invsq2pi / par[3]);
}



TF1 *langaufit(TH1D *his, double *fitrange, double *startvalues, double *parlimitslo, double *parlimitshi, double *fitparams, double *fiterrors, double *ChiSqr, int *NDF)
{
    // Once again, here are the Landau * Gaussian parameters:
    //   par[0]=Width (scale) parameter of Landau density
    //   par[1]=Most Probable (MP, location) parameter of Landau density
    //   par[2]=Total area (integral -inf to inf, normalization constant)
    //   par[3]=Width (sigma) of convoluted Gaussian function
    //
    // Variables for langaufit call:
    //   his             histogram to fit
    //   fitrange[2]     lo and hi boundaries of fit range
    //   startvalues[4]  reasonable start values for the fit
    //   parlimitslo[4]  lower parameter limits
    //   parlimitshi[4]  upper parameter limits
    //   fitparams[4]    returns the final fit parameters
    //   fiterrors[4]    returns the final fit errors
    //   ChiSqr          returns the chi square
    //   NDF             returns ndf

    int i;
    char FunName[100];

    sprintf(FunName,"Fitfcn_%s",his->GetName());

    TF1 *ffitold = (TF1*)gROOT->GetListOfFunctions()->FindObject(FunName);
    if (ffitold) delete ffitold;

    TF1 *ffit = new TF1(FunName,langaufun,fitrange[0],fitrange[1],4);
    ffit->SetParameters(startvalues);
    ffit->SetParNames("Width","MP","Area","GSigma");

    for (i=0; i<4; i++) {
        ffit->SetParLimits(i, parlimitslo[i], parlimitshi[i]);
    }

    his->Fit(FunName,"RB0");   // fit within specified range, use ParLimits, do not plot

    ffit->GetParameters(fitparams);    // obtain fit parameters
    for (i=0; i<4; i++) {
        fiterrors[i] = ffit->GetParError(i);     // obtain fit parameter errors
    }
    ChiSqr[0] = ffit->GetChisquare();  // obtain chi^2
    NDF[0] = ffit->GetNDF();           // obtain ndf

    return (ffit);              // return fit function

}


int langaupro(double *params, double &maxx, double &FWHM) {

    // Searches for the location (x value) at the maximum of the
    // Landau-Gaussian convolute and its full width at half-maximum.
    //
    // The search is probably not very efficient, but it's a first try.

    double p,x,fy,fxr,fxl;
    double step;
    double l,lold;
    int i = 0;
    int MAXCALLS = 10000;


    // Search for maximum

    p = params[1] - 0.1 * params[0];
    step = 0.05 * params[0];
    lold = -2.0;
    l    = -1.0;


    while ( (l != lold) && (i < MAXCALLS) ) {
        i++;

        lold = l;
        x = p + step;
        l = langaufun(&x,params);

        if (l < lold)
            step = -step/10;

        p += step;
    }

    if (i == MAXCALLS)
        return (-1);

    maxx = x;

    fy = l/2;


    // Search for right x location of fy

    p = maxx + params[0];
    step = params[0];
    lold = -2.0;
    l    = -1e300;
    i    = 0;


    while ( (l != lold) && (i < MAXCALLS) ) {
        i++;

        lold = l;
        x = p + step;
        l = TMath::Abs(langaufun(&x,params) - fy);

        if (l > lold)
            step = -step/10;

        p += step;
    }

    if (i == MAXCALLS)
        return (-2);

    fxr = x;


    // Search for left x location of fy

    p = maxx - 0.5 * params[0];
    step = -params[0];
    lold = -2.0;
    l    = -1e300;
    i    = 0;

    while ( (l != lold) && (i < MAXCALLS) ) {
        i++;

        lold = l;
        x = p + step;
        l = TMath::Abs(langaufun(&x,params) - fy);

        if (l > lold)
            step = -step/10;

        p += step;
    }

    if (i == MAXCALLS)
        return (-3);


    fxl = x;

    FWHM = fxr - fxl;
    return (0);
}

void plot_main_det() {
    // TFile *f = new TFile("/Users/sudip/Documents/Academics/Research/moller/analysis/beamtest-halld/data/rootfiles/production/Ring5_2001_132657_head_1evios.root","READ");//change the name of the root file you want to open
    TFile *f = new TFile("/Users/sudip/Documents/Academics/Research/moller/analysis/beamtest-halld/data/rootfiles/production/Ring5_2029_132729_head_20evios.root","READ");
    TTree *t=(TTree*)f->Get("tree1");
    TH1D *hist = new TH1D("hist", "Pulse height distributio of R5 from halld testbeam;number of PEs;Counts", 100, 0, 300);

    double langaufun(double , double );
    TF1 *langaufit(TH1D *his, double *fitrange, double *startvalues, double *parlimitslo, double *parlimitshi, double *fitparams, double *fiterrors, double *ChiSqr, int *NDF);
    t->Draw("(ch_15*0.02)/(5.0*0.21) >> hist", "amp_15>0&&tiler==85&&tiler", "goff");
    double fr[2];
    double sv[4], pllo[4], plhi[4], fp[4], fpe[4];
    fr[0]=15.0;
    fr[1]=100.0;
    pllo[0]=1; pllo[1]=1.0; pllo[2]=1.0; pllo[3]=1.0;
    plhi[0]=15.0; plhi[1]=100.0; plhi[2]=hist->Integral()*5; plhi[3]=50.0;
    sv[0]=2.3; sv[1]=35.0; sv[2]=29000.0; sv[3]=19.0;

    double chisqr;
    int    ndf;
    TF1 *fitsnr = langaufit(hist,fr,sv,pllo,plhi,fp,fpe,&chisqr,&ndf);

    double SNRPeak, SNRFWHM;
    langaupro(fp,SNRPeak,SNRFWHM);

    printf("Fitting done\nPlotting results...\n");

    // Global style settings
    gStyle->SetOptStat("eMR");
    gStyle->SetOptFit(111);
    gStyle->SetLabelSize(0.03,"x");
    gStyle->SetLabelSize(0.03,"y");

    TCanvas *c2_langau_ff = new TCanvas();
    hist->Draw("hist E");

    fitsnr->Draw("lsame");

    // Print RMS/Mean in the pad
    TLatex *tex = new TLatex();
    tex->SetTextSize(0.04);
    tex->DrawLatexNDC(0.65,0.4,Form("PE yield = %.2f", fitsnr->GetParameter(1)));
    tex->DrawLatexNDC(0.65,0.3,Form("Resolution = %.2f", fitsnr->GetParameter(3)/fitsnr->GetParameter(1)));
    tex->DrawLatexNDC(0.65,0.2,Form("RMS/Mean = %.2f", hist->GetRMS()/hist->GetMean()));

}
