// Script: replay_testbeam.C
// Description: This script reads the rootfiles that halld software (hd_root) generates 
//              and plots the amplitude and integral histograms.
// Author: Sudip Bhattarai (sudip@jlab.org)
// Date: 2025-05-03 10:54


#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <regex>

#include "RtypesCore.h"
#include "TH1.h"
#include "TLegend.h"
#include "TMath.h"
#include "TString.h"
#include "TStyle.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TF1.h"
#include "TVirtualPad.h"
#include "TProfile.h"
#include "ROOT/RDataFrame.hxx"
#include "TLatex.h"
#include "TKey.h"
#include "TPaveStats.h"
#include "TSystem.h"
#include "TTreeReaderArray.h"


// #include "./plot_utilities.hh"
// #include "./langaus.hh"

using namespace std;

// =================================================
// Structs
// =================================================
struct RunInfo {
    string det;
    int run_num;
    double x_pos; // x position of the motion control system
    double y_pos; // y positio of the motion control system
    int pmt; // PMT serial number
    int hv; // High voltage
    int run_phase; // Run phase not week number
    int adc_amp_max; // Maximum of the ADC amplitudes (amp_15)
    int adc_integral_max; // Maximum of the ADC integrals (ch_15)
    int adc_amp_min_fit; // Minimum of the ADC amplitudes (amp_15) used for the fit
};

// =================================================
// Function prototypes
// =================================================
vector<double> get_PS_energy_vector(string filename);
vector<double> get_energy_bins(string filename);
vector<double> get_hods_hist_bins();
vector<double> get_hods_width_bins();
ROOT::RDF::RNode get_dataframe(const std::string& filename);
std::pair<double, double> get_position_from_log(int target_run);
double get_pmt_gain(int pmt_num, int HV);
int extract_run_number_from_path(const std::string& filepath);
double get_npe_from_adc(float adcValue, double pmtgain);
void plot_replay(ROOT::RDF::RNode df, int target_run);
void plot_adc_pulse_height_distribution(ROOT::RDF::RNode df, int target_run);
void plot_npe_pulse_height_distribution(ROOT::RDF::RNode df, int target_run);
void plot_resolution_vs_energy(ROOT::RDF::RNode df);
void plot_energy_normalized_adc(ROOT::RDF::RNode df, int target_run);
void plot_waveform_profiles(const string& waveform_rootfile);
void plot_waveform_arrays(const string& waveform_rootfile);
void plot_gain_calibration_raw_amp();
void plot_gain_calibration_single_pe();
RunInfo get_run_info(int target_run);
void plot_compare_sim_testbeam(std::string sim_rootfile, ROOT::RDF::RNode df, int target_run, double beamEnergy);
void plot_compare_simLP_testbeam(std::string sim_rootfile, ROOT::RDF::RNode df, int target_run, double beamEnergy);
void plot_energy_normalized_response_slope();

// =================================================
// Global variables
// =================================================
int color[] = {kBlack, kRed, kGreen+2, kBlue, kMagenta};

string source_dir = "/Users/sudip/moller/analysis/beamtest-halld/";
string ps_energy_file = "data/inputs/PS_energy.txt";
string run_log = "data/inputs/run_settings_log.csv";
string pmt_gain_data = "data/inputs/pmt_gain.csv";
string rootfile_default = "data/rootfiles/production/Showermax_1061_131659_head_15evios.root";
string waveform_rootfile_generic = "data/rootfiles/archive/Showermax_131445_waveform.root";
string gain_calibration_rootfile_lg = "data/rootfiles/archive/Showermax_1117_132102_lg.root";
string gain_calibration_rootfile_tq = "data/rootfiles/archive/Showermax_1117_132102_tq.root";
string simrootfile = "data/sim/testbeam_energies_hitn_showerMaxDetector_v3-1-2_positron.root";
string enorm_fit_params = "data/outfiles/enorm_fit_params.csv";

// =================================================
// Main function
// =================================================
void analyze_testbeam(const string& filepath = rootfile_default) {
    const string file = source_dir + filepath;
    int run = extract_run_number_from_path(filepath);

    //## See raw wavefrom waveform
    // plot_waveform_profiles(waveform_rootfile_generic);
    // plot_waveform_arrays(file);

    //## See the gain calibration run
    // plot_gain_calibration_raw_amp();
    // plot_gain_calibration_single_pe();

    //## Make RDataFrame
    ROOT::RDF::RNode df = get_dataframe(file);
    // df.Display({"tilel", "tiler", "amp_15", "ch_15", "tiler_e", "tiler_pos", "tiler_npe"},10,10)->Print();

    //## Plot replay
    plot_replay(df, run); // 1 canvas with 3x2 pads
    // plot_energy_normalized_adc(df, run); // 1 canvas

    //## Plot adc/resolution vs energy from gaussian fit
    // plot_resolution_vs_energy(df); // 1 canvas with 2x1 pads

    //## Plot pulse height distribution (for cin energy value)
    // plot_adc_pulse_height_distribution(df, run);
    plot_npe_pulse_height_distribution(df, run);

    //## Compare with simulation
    // plot_compare_sim_testbeam(simrootfile, df, run, 4.5);
    // plot_compare_simLP_testbeam(simrootfile, df, run, 4.5);

    //## Plot energy normalized response slope
    // plot_energy_normalized_response_slope();
    // Test functions

}

// =================================================
// Function definitions
// =================================================

/* @brief: Read the run log file and creates the run info struct base on the target run
 * @param target_run: Target run number
 * @return: RunInfo struct 
 */
RunInfo get_run_info(int target_run) {
    std::ifstream file(run_log);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Could not open file: " << run_log << std::endl;
        exit(1);
    }


    std::string line;
    // Skip header
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        // Parse fields
        std::getline(ss, token, ',');  // run_num
        int run_num = std::stoi(token);

        if (run_num == target_run) {
            RunInfo info;
            
            info.run_num = run_num;

            std::getline(ss, token, ',');  // x_pos
            info.x_pos = std::stod(token);

            std::getline(ss, token, ',');  // y_pos
            info.y_pos = std::stod(token);

            std::getline(ss, token, ',');  // pmt
            info.pmt = std::stoi(token);

            std::getline(ss, token, ',');  // HV
            info.hv = std::stoi(token); 

            // Determine run phase based on run number
            if (run_num >= 1003 && run_num <= 1042) {
                info.run_phase = 1;
                info.adc_amp_max = 2000;
                info.adc_integral_max = 15000;
                info.adc_amp_min_fit = 70;
            } else if (run_num >= 1043 && run_num <= 1059) {
                info.run_phase = 2;
                info.adc_amp_max = 500;
                info.adc_integral_max = 3000;
                info.adc_amp_min_fit = 70;
            } else if (run_num >= 1060 && run_num <= 1116) {
                info.run_phase = 3;
                info.adc_amp_max = 1500;
                info.adc_integral_max = 10000;
                info.adc_amp_min_fit = 70;
            } else if (run_num >= 1117 && run_num <= 1118) {
                info.run_phase = 4;
                info.adc_amp_max = 1500;
                info.adc_integral_max = 10000;
                info.adc_amp_min_fit = 70;
            } else if (run_num >= 1119 && run_num <= 1122) {
                info.run_phase = 5;
                info.adc_amp_max = 5000;
                info.adc_integral_max = 30000;
                info.adc_amp_min_fit = 70;
            } else if (run_num >= 2000 && run_num <= 2030) {
                info.run_phase = 6;
                info.adc_amp_max = 1000;
                info.adc_integral_max = 10000;
                info.adc_amp_min_fit = 70;
            } else {
                std::cerr << "[WARNING] Run number " << run_num << " does not match any defined run phase.\n";
                info.run_phase = -1; // or 0 if undefined
            }
            return info;
        }
    }

    std::cerr << "[ERROR] Run number " << target_run << " not found in file: " << run_log << std::endl;
    exit(1);
}

/* @brief: Creates a ROOT::RDF::RNode from a ROOT filename
 * @note: ROOT::RDataFrame is the initial entry point for RDF, but once you call
 *        .Define(...) or .Filter(...), the return type becomes a ROOT::RDF::RNode.
 * @params: string filepath of the root filename
 * @returns: ROOT::RDF::RNode
 */
ROOT::RDF::RNode get_dataframe(const std::string& filename) {
    // Create initial DataFrame from tree
    ROOT::RDataFrame df("tree1", filename);

    // Load external lookup vectors
    std::vector<double> ps_energy = get_PS_energy_vector(source_dir + ps_energy_file);
    std::vector<double> hods_width_bins = get_hods_width_bins();

    // Calculate midpoints of hods_width_bins
    std::vector<double> hods_width_mid;
    for (int i = 0; i < hods_width_bins.size() -1; i++) {
        hods_width_mid.push_back((hods_width_bins[i] + hods_width_bins[i + 1]) / 2.0);
    }

    // Get runnumber from filename
    int run_num = extract_run_number_from_path(filename);
    RunInfo info = get_run_info(run_num);
    double pmt_gain = get_pmt_gain(info.pmt, info.hv);


    // Define tiler_e, tiler_pos and tiler_npe using lambdas
    auto df_extended = df
        .Define("tiler_e", [=](int tiler) {
            if (tiler > 0 && static_cast<size_t>(tiler) <= ps_energy.size())
                return ps_energy[tiler - 1];
            else
                return -1.0;
        }, {"tiler"})
        .Define("tiler_pos", [=](int tiler) {
            if (tiler > 0 && static_cast<size_t>(tiler) <= hods_width_mid.size())
                return hods_width_mid[tiler - 1];
            else
                return -1.0;
        }, {"tiler"})
        .Define("tiler_npe", [=](float adcValue) {
            return get_npe_from_adc(adcValue, pmt_gain);
        }, {"ch_15"});


    return df_extended;
}

/* @brief: Read the PS_energy.txt file and return the energy bins
 *          to use for histograms
 * @params: Path name of the energy bins file (PS_energy.txt)
 * @return: vector of energy nBinsX
 */
vector<double> get_energy_bins(string filename) {
    ifstream infile(filename);
    vector<double> energy_bins;

    if (!infile.is_open()) {
        cerr << "Cannot open file: " << filename << endl;
        return energy_bins;
    }

    string line;
    int line_count = 0;

    // Skip first 4 lines
    while (line_count < 4 && getline(infile, line)) {
        line_count++;
    }


    double last_max = 0.0;
    bool first = true;

    // Read the energy ranges
    while (getline(infile, line)) {
        istringstream ss(line);
        double e_min, e_max;
        if (ss >> e_min >> e_max) {
            if (first) {
                energy_bins.push_back(e_min);  // Push only the first min
                first = false;
            }
            energy_bins.push_back(e_max);    // Always push max
        }
        line_count++;
    }

    infile.close();

    // Now make a custom bins 
    std::vector<double> custom_bins;
    for (int i = 0; i <= 105; ++i)
        custom_bins.push_back(energy_bins[i]);
    for (int i = 106; i <= 145; i += 2)
        custom_bins.push_back(energy_bins[i]);

    return custom_bins;
}

/* @brief: Reads the PS_energy.txt file and return the energy energy_averages
 *          to use for histograms weighting
 * @params: Path name of the energy bins file (PS_energy.txt)
 * @return: vector of energy_averages
 */
vector<double> get_PS_energy_vector(const string filename) {
    ifstream infile(filename);
    vector<double> energy_averages;

    if (!infile.is_open()) {
        cerr << "Cannot open file: " << filename << endl;
        return energy_averages;
    }

    string line;
    int line_count = 0;

    // Skip first 4 lines
    while (line_count < 4 && getline(infile, line)) {
        line_count++;
    }

    // Process each line
    while (getline(infile, line)) {
        istringstream ss(line);
        double val1, val2;
        if (ss >> val1 >> val2) {
            double avg = 0.5 * (val1 + val2);
            energy_averages.push_back(avg);
        }
    }

    infile.close();
    return energy_averages;
}

/* @brief: Makes vector of Hodoscope historam bins of width 2mm
 * @note:  Hodoscope columns 1-105 are 2mm tiles with 26 MeV wide 
 *         columns 106-145 are 1mm tiles are 13 MeV wide
 * @params: None
 * @return: vector of hods
 */
vector<double> get_hods_hist_bins(){
    vector<double> hods;
    for (int i = 1; i <= 125; i++) {
        hods.push_back(i*2);
    }
    return hods;
}

/* @brief: Creates the vector of the histogram bins of the hodoscope in terms of width in mm
 *         Maps the hosdoscope number with its position (0 mm is left most hodoscope's left edge)
 * @note:  Hodoscope columns 1-105 are 2mm tiles with 26 MeV wide columns 106-145 are 1mm tiles are 13 MeV wide
 * @params: None
 * @return: vector of hods width in mm
 */
vector<double> get_hods_width_bins(){
    int n2mmHods = 105;
    int n1mmHods = 40; 
    vector<double> hods;

    for (int i = 1; i <= n2mmHods; i++) {
        hods.push_back(i*2 - 2); // 2mm width starts from 0 mm
    }
    for (int i = 1; i <= n1mmHods+1; i++) {
        hods.push_back(n2mmHods*2 + i*1 - 1);
    }    

    return hods;
}

/*
 * @brief: Read the run_log.csv file and return the position of the target run
 * @params: Target run number
 * @return: pair of x and y position
 */
std::pair<double, double> get_position_from_log(int target_run) {
    std::string run_log_path = source_dir + run_log;
    std::ifstream file(run_log_path);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open file: " << run_log_path << std::endl;
        return {0.0, 0.0};
    }

    std::string line;
    // Skip the header
    std::getline(file, line);

    // Process each line
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        // Read columns
        std::getline(ss, token, ','); // run_num
        int run_num = std::stoi(token);

        if (run_num == target_run) {
            std::getline(ss, token, ','); // x_pos
            double x_pos = std::stod(token);
            std::getline(ss, token, ','); // y_pos
            double y_pos = std::stod(token);

            return {x_pos, y_pos};
        }
    }

    std::cerr << "[WARNING] Run number " << target_run << " not found in " << run_log_path << std::endl;
    return {0.0, 0.0};  // default fallback
}

/*
 * @brief: Read the pmt_gain.csv file and return the gain value for 
 *         the target pmt and hv
 * @params: pmt number and high voltage
 * @return: The pmt gain value
 */
double get_pmt_gain(int pmt_num, int hv) {
    std::ifstream infile(pmt_gain_data.c_str());
    if (!infile.is_open()) {
        std::cerr << "Error: Cannot open file " << pmt_gain_data << std::endl;
        exit(1);
    }

    std::string line;

    // Skip header line once
    if (!std::getline(infile, line)) {
        std::cerr << "Error: File is empty or has no header." << std::endl;
        return -1.0;
    }

    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string token;
        int pmtNum, HV;
        double gain;

        // Parse pmtNum
        if (!std::getline(ss, token, ',')) continue;
        pmtNum = std::stoi(token);

        // Parse HV
        if (!std::getline(ss, token, ',')) continue;
        HV = std::stoi(token);

        // Parse gain
        if (!std::getline(ss, token, ',')) continue;
        gain = std::stod(token);

        if (pmtNum == pmt_num && HV == hv) {
            infile.close();
            return gain;
        }
    }

    infile.close();
    std::cerr << "Warning: No matching entry found for PMT " << pmt_num << " and HV " << hv << std::endl;
    return -1.0;
}

/*
 * @brief: Extract the run number from the rootfile Path
 * @params: File Path in string
 * @return: Run number in interger
 */
int extract_run_number_from_path(const std::string& filepath) {
    // Adjusted regex: removed hyphen, fixed typo
    std::regex pattern(R"((Showermax|Ring5)_(\d+))");
    std::smatch match;

    if (std::regex_search(filepath, match, pattern) && match.size() > 1) {
        std::string run_str = match[2].str();  // Convert to string explicitly
        return std::stoi(match[2]); // match[1] is the run number
    } else {
        std::cerr << "[ERROR] Could not extract run number from: " << filepath << std::endl;
        return -1;
    }
}

/*
 * @brief: Convert fADC value to number of photoelectrons based on PMT gain
 * @params: ADC value, PMT gain
 * @return: NPE Values (calculates to be ADC * 0.122 / Gain in Millions)
 */
double get_npe_from_adc(float adcValue, double pmtgain){
    double fadcRange = 1.0;             // fADC dynamic range in V
    double vRes = fadcRange / 4096.0;   // 12 bits
    double impedence = 50;              // 50 Ohms
    double adcTimeRes = 4e-9;           // 250 MHz sampling rate
    double eCharge = 1.6e-19;
    double qRes = vRes / impedence * adcTimeRes;
    double npe = (adcValue * qRes) / (pmtgain * eCharge);
    return npe;                         // 
}

/**
 * @brief: Create a 2x3 multipad canvas and draw amplitude and integral histograms.
 *         It is used to make sure the data is good and reasonable in the first look.  
 * @param: RDataFrame df node that contains the energy column
 * @return: None, plots the canvas in the ../data/plots/replays directory
 */
void plot_replay(ROOT::RDF::RNode df, int target_run) {
    // Style
    // gStyle->SetOptStat("neMR");  // Disable globally
    // gStyle->SetOptFit(1);
    gStyle->SetAxisMaxDigits(3);
    // gStyle->SetPadLeftMargin(0.15);
    // gStyle->SetPadRightMargin(0.15);

    // Run Info
    RunInfo info = get_run_info(target_run);
    double x_pos = info.x_pos;
    double y_pos = info.y_pos;
    double pmt = info.pmt;
    double hv = info.hv;
    double gain = get_pmt_gain(pmt, hv);
    std::pair<double, double> position = {x_pos, y_pos};

    // Create canvas and divide into 3x2 grid
    auto c = new TCanvas("c", Form("Replay Plot: Run_%d Pos(%.0f, %.0f)", target_run, position.first, position.second), 1200, 600); // 2800, 1500);
    c->Divide(3, 2);
    c->SetRightMargin(0.30);

    // Histogram settings
    double ampMin_cut = 10;
    double ampMin = ampMin_cut, ampMax = info.adc_amp_max+ampMin_cut;
    int nBinsAmp = 100;
    double chMin_cut = 100;
    double chMin = chMin_cut, chMax = info.adc_integral_max+chMin_cut;
    int nBinsCh = 100;
    int nBinsPS = 145;
    double psMin = 0, psMax = 145;
    vector<double> energy_bins = get_energy_bins(source_dir + ps_energy_file);
    int nBinsE = energy_bins.size() - 1; // 145
    vector<double> hods_bins = get_hods_hist_bins();
    int nBinsHod = hods_bins.size() - 1;


    // Pad 1: TH1D of amp_15
    gStyle->SetOptStat("eMR");  // Disable globally
    c->cd(1);
    gPad->SetGrid();

    auto h_amp = df.Histo1D({"hist_amp", Form("Amplitude: Run_%d Pos(%.0f, %.0f);ADC;Entries", target_run, position.first, position.second), nBinsAmp, ampMin, ampMax}, "amp_15");

    // Find peak bin and center
    int peak_bin_amp = h_amp->GetMaximumBin();
    double peak_adc_amp = h_amp->GetBinCenter(peak_bin_amp);

    // Define fitting range around peak
    double fit_min_amp = peak_adc_amp - 400;
    double fit_max_amp = peak_adc_amp + 400;

    TH1D* h_amp_clone = (TH1D*)h_amp->Clone("hist_amplitude");
    h_amp_clone->Draw();
    c->Update();

    // Fit Gaussian
    // TF1 *gaus_fit = new TF1("gaus_fit", "gaus", fit_min_amp, fit_max_amp);
    // gaus_fit->SetLineColor(kRed);
    // h_amp->Fit(gaus_fit, "RQ");  // Quiet and respect range
    // gaus_fit->Draw("same");
    h_amp_clone->Draw("hist E");
    
    // Move stat box
    TPaveStats* stats1 = (TPaveStats*)h_amp_clone->FindObject("stats");
    if (stats1) {
        stats1->SetX1NDC(0.7);
        stats1->SetX2NDC(0.9);
        stats1->SetY1NDC(0.7);
        stats1->SetY2NDC(0.9);
        stats1->SetTextSize(0.03);
    }

    c->Update();


    // Pad 2: TH2D of amp_15 vs tiler_e
    gStyle->SetOptStat(0);
    c->cd(2);
    gPad->SetGrid();
    gPad->SetRightMargin(0.15);

    auto h2_amp = df.Histo2D({"h2_amp",  Form("Amplitude: Run_%d Pos(%.0f, %.0f);Hosdoscope position [mm];ADC Values;Entries", target_run, position.first, position.second), nBinsHod, hods_bins.data(), nBinsAmp, ampMin, ampMax}, "tiler_pos", "amp_15");
    // h2_amp->SetAxisRange(0, 250, "X");
    h2_amp->DrawClone("COLZ");

    c->Update();

    // Pad 3: TProfile of amp_15 vs tiler_e
    c->cd(3);
    gPad->SetGrid();

    auto prof_amp = df.Profile1D({"prof_amp", "Profile: ADC amplitude vs Hodoscope position;Hodoscope position [mm];Mean ADC", nBinsHod, hods_bins.data()}, "tiler_pos", "amp_15");

    // Find first bin where mean > 500
    const double threshold = info.adc_amp_min_fit;
    const int edge_offset = 1;
    int profFitStartBin;
    int profFitEndBin;
    int firstBinAboveThreshold = -1;
    int lastBinAboveThreshold = -1;
    int nBins = prof_amp->GetNbinsX();

    for (int i = 1; i <= nBins; ++i) {
        if (prof_amp->GetBinContent(i) > threshold) {
            firstBinAboveThreshold = i;
            break;
        }
    }

    if (firstBinAboveThreshold == -1) {
        std::cerr << "[pad3] Warning: No bin with mean > threshold(500). Skipping fit." << std::endl;
    } else {
        // Loop backward from the last bin to find where mean value starts above threshold
        for (int i = nBins; i <= nBins; i--) {
            if (prof_amp->GetBinContent(i) > threshold) {
                lastBinAboveThreshold = i;
                break;
            }
        }

        profFitStartBin = firstBinAboveThreshold + edge_offset;
        profFitEndBin = lastBinAboveThreshold - edge_offset;

        double x1 = prof_amp->GetBinLowEdge(profFitStartBin);
        double x2 = prof_amp->GetBinLowEdge(profFitEndBin + 1);

        TF1 *fit1 = new TF1("fit1", "pol1", x1, x2);
        prof_amp->Fit(fit1, "RQ");  // R = respect range, Q = quiet
        fit1->SetLineColor(kRed);
        prof_amp->DrawClone("E1");
        fit1->Draw("same");
    }

    prof_amp->DrawClone("E1");

    c->Update();

    // Pad 4: TH1D of ch_15
    gStyle->SetOptStat("eMR");
    // gStyle->SetOptFit(1);
    c->cd(4);
    gPad->SetGrid();

    auto h_ch = df.Histo1D({"h_ch", Form("Integral: Run_%d Pos(%.0f, %.0f);ADC;Entries", target_run, position.first, position.second), nBinsCh, chMin, chMax}, "ch_15");
    h_ch->Scale(1.0 / h_ch->Integral());

    // Find peak bin and center
    int peak_bin_ch = h_ch->GetMaximumBin();
    double peak_adc_ch = h_ch->GetBinCenter(peak_bin_ch);

    // Define fitting range around peak
    double fit_min_ch = peak_adc_ch - 2000;
    double fit_max_ch = peak_adc_ch + 2000;

    TH1D* h_ch_clone = (TH1D*)h_ch->Clone("hist_ch_clone");
    h_ch_clone->Draw();
    c->Update();

    // Fit Gaussian
    // TF1 *gaus_fit_ch = new TF1("gaus_fit", "gaus", fit_min_ch, fit_max_ch);
    // gaus_fit->SetLineColor(kRed);
    // h_ch->Fit(gaus_fit_ch, "RQ");  // Quiet and respect range
    // h_ch->DrawClone("hist E");
    // gaus_fit->Draw("same");
    h_ch_clone->Draw("hist E");

    // Move stat box
    TPaveStats* stats4 = (TPaveStats*)h_ch_clone->FindObject("stats");
    if (stats4) {
        stats4->SetX1NDC(0.7);
        stats4->SetX2NDC(0.9);
        stats4->SetY1NDC(0.7);
        stats4->SetY2NDC(0.9);
        stats4->SetTextSize(0.03);
    }

    c->Update();

    // Pad 5: TH2D of ch_15 vs tiler_e
    gStyle->SetOptStat(0);
    c->cd(5);
    gPad->SetGrid();
    gPad->SetRightMargin(0.15);

    auto h2_ch = df.Histo2D({"h2_ch", "ADC Integral vs Energy;Energy (GeV);ADC Integral;Entries", nBinsE, energy_bins.data(), nBinsCh, chMin, chMax}, "tiler_e", "ch_15");
    h2_ch->DrawClone("COLZ");

    gPad->Update();

    // Pad 6: TProfile of ch_15 vs tiler_e
    gStyle->SetOptStat(1110);
    gStyle->SetOptFit(1);
    c->cd(6);
    gPad->SetGrid();

    auto prof_ch = df.Profile1D({"prof_ch", "Profile: ch_15 vs Energy;Energy (GeV);Mean Integral", nBinsE, energy_bins.data()}, "tiler_e", "ch_15");

    // Clone the profile to preserve it across draws
    TProfile* prof_ch_clone = (TProfile*)prof_ch->Clone("prof_ch_copy");
    prof_ch_clone->SetAxisRange(chMin, chMax/2, "Y");

    double x1 = prof_ch->GetBinLowEdge(profFitStartBin);
    double x2 = prof_ch->GetBinLowEdge(profFitEndBin);

    TF1 *fit1 = new TF1("fit1", "pol1", x1, x2);
    fit1->SetLineColor(kRed);
    fit1->SetLineWidth(2);

    prof_ch_clone->Draw();
    c->Update();  // Needed to generate the stat box

    prof_ch_clone->Fit(fit1, "RQ");
    fit1->Draw("same");
    prof_ch_clone->Draw("same E1");

    // Move stat box
    TPaveStats* stats6 = (TPaveStats*)prof_ch_clone->FindObject("stats");
    if (stats6) {
        stats6->SetX1NDC(0.55);
        stats6->SetX2NDC(0.9);
        stats6->SetY1NDC(0.1);
        stats6->SetY2NDC(0.45);
        stats6->SetTextSize(0.03);
    }

    c->Modified();

    // Save plots
    string plot_name = Form("%sdata/plots/replays/run_%d_X%.0f_Y%.0f.pdf", source_dir.c_str(), target_run, position.first, position.second);
    c->SaveAs(plot_name.c_str());
    c->Update();
}

/* @brief: Plots a pulse height distribution of ADC value for a given energy
* @note: If energy entered is not between 3.00 and 6.00 GeV, the function exits.
* @params: 1: RDataFrame (RDF::RNode to be exact) that contains the energy column, 
*          2: the local run number (1XXX), not halld run number
* @returns: None
*/ 
void plot_adc_pulse_height_distribution(ROOT::RDF::RNode df, int run) {
    // Styles
    gStyle->SetOptStat("eMR");
    gStyle->SetOptFit(1);

    // Energy
    double energy_input = 4.5;  // GeV
    std::cout << "For pulse height distribution, enter energy value in GeV [3.00 - 6.25]: ";
    std::cin >> energy_input;

    if (energy_input < 3.0 || energy_input > 6.5) {
        std::cerr << "Invalid energy value. Exiting..." << std::endl;
        exit(1);
    }

    double energy_window = 0.010;  // 26 MeV = 0.026 GeV
    double e_min = energy_input - energy_window;
    double e_max = energy_input + energy_window;

    // // Shower-max postions
    // pair<double, double> position = get_position_from_log(run);
    // double x = position.first;
    // double y = position.second;

    // Run info
    RunInfo info = get_run_info(run);

    auto filtered_df = df.Filter([=](double e) { return e >= e_min && e <= e_max; }, {"tiler_e"});

    auto hist = filtered_df.Histo1D(
        {"hist_integral", Form("Run: %d, Pos: (%.0f, %.0f), E = %.3f #pm 0.026 GeV;Integral ADC;Counts", run, info.x_pos , info.y_pos, energy_input), 
            100, 0, (double)info.adc_integral_max}, "ch_15");

    // Peak and fit
    int peak_bin = hist->GetMaximumBin();
    double peak_center = hist->GetBinCenter(peak_bin);
    double fit_min = std::max(100.0, peak_center - 2000);
    double fit_max = std::min(30000.0, peak_center + 2000);

    // Plot
    TCanvas *c = new TCanvas("c_ch15", "Pulse Height Distribution", 800, 600);
    gPad->SetGrid();
    TF1 *gaus_fit = new TF1("gaus_fit", "gaus", fit_min, fit_max);
    hist->Fit(gaus_fit, "RQ");
    gaus_fit->SetLineColor(kRed);
    gaus_fit->Draw("same");
    hist->DrawClone("hist E");

    // Add text for resolution
    TLatex *latex = new TLatex();
    latex->SetNDC();
    latex->SetTextSize(0.04);
    double res = gaus_fit->GetParameter(2)/gaus_fit->GetParameter(1);
    latex->DrawLatex(0.62, 0.55, Form("#frac{RMS}{Mean} = %.2f", res));  // Below stat box

    c->Update();

    // Save plots
    string plot_name = Form("%sdata/plots/pulse-heights/pulse_height_adc_run_%d_%.3fGeV.pdf", source_dir.c_str(), run, energy_input);
    c->SaveAs(plot_name.c_str());
    c->Update();
}


/* @brief: Plots a pulse height distribution of number of PEs for a given energy
* @note: If energy entered is not between 3.00 and 6.00 GeV, the function exits.
* @params: 1: RDataFrame (RDF::RNode to be exact) that contains the energy column, 
*          2: the local run number (1XXX), not halld run number
* @returns: None
*/ 
void plot_npe_pulse_height_distribution(ROOT::RDF::RNode df, int run) {
    // Styles
    gStyle->SetOptStat("eMR");
    gStyle->SetOptFit(1);

    // Run info
    RunInfo run_info = get_run_info(run);

    // Energy
    double energy_input = 4.5;  // GeV
    std::cout << "For pulse height distribution, enter energy value in GeV [3.00 - 6.25]: ";
    std::cin >> energy_input;

    if (energy_input < 3.0 || energy_input > 6.5) {
        std::cerr << "Invalid energy value. Continuing..." << std::endl;
        return;
    }

    double energy_window = 0.026;  // 26 MeV = 0.026 GeV
    double e_min = energy_input - energy_window;
    double e_max = energy_input + energy_window;

    // Shower-max postions
    pair<double, double> position = get_position_from_log(run);
    double x = run_info.x_pos;
    double y = run_info.y_pos;

    auto filtered_df = df.Filter([=](double e) { return e >= e_min && e <= e_max; }, {"tiler_e"});
    // filtered_df.Display({"tiler", "tiler_e", "ch_15", "tiler_npe"}, 50, 50)->Print();

    auto hist = filtered_df.Histo1D(
        {"hist_nPEs", Form("Pulse height for Run: %d, Pos: (%.0f, %.0f), E = %.3f #pm %.3f GeV;Number of PEs;Counts", run, x, y, energy_input, energy_window), 
            200, 1, 1001}, "tiler_npe");

    // Peak and fit
    int peak_bin = hist->GetMaximumBin();
    double peak_center = hist->GetBinCenter(peak_bin);
    double fit_min = std::max(1.0, peak_center - 100);
    double fit_max = std::min(500.0, peak_center + 100);

    // Plot
    TCanvas *c = new TCanvas("c_ch15", "Pulse Height Distribution of PEs", 800, 600);
    TF1 *gaus_fit = new TF1("gaus_fit", "gaus", fit_min, fit_max);
    hist->Fit(gaus_fit, "RQ");
    gaus_fit->SetLineColor(kRed);
    gaus_fit->Draw("same");
    hist->DrawClone("hist E");

    // Add text for resolution
    TLatex *latex = new TLatex();
    latex->SetNDC();
    latex->SetTextSize(0.04);
    double res = gaus_fit->GetParameter(2)/gaus_fit->GetParameter(1);
    double mean_err_frac = gaus_fit->GetParError(1)/gaus_fit->GetParameter(1);
    double rms_err_frac = gaus_fit->GetParError(2)/gaus_fit->GetParameter(2);

    double res_err = TMath::Sqrt(mean_err_frac*mean_err_frac + rms_err_frac*rms_err_frac);
    latex->DrawLatex(0.62, 0.50, Form("#frac{RMS}{Mean} = %.2f #pm %.2f", res, res_err));  // Below stat box
    latex->DrawLatex(0.62, 0.40, Form("E response = %.2f PEs/GeV", gaus_fit->GetParameter(1)/energy_input));
    latex->DrawLatex(0.62, 0.30, Form("PMT gain = %.0f", get_pmt_gain(run_info.pmt, run_info.hv)));

    c->Update();

    // // Open the output csv file and rewrite the fit parameters
    // std::ofstream output_file(Form("%sdata/outfiles/enorm_fit_params.csv", source_dir.c_str()), std::ios::app);
    // output_file << run << "," << run_info.x_pos << "," << run_info.y_pos << "," <<
    //     fit->GetParameter(0) << "," << fit->GetParError(0) << "," << 
    //     fit->GetParameter(1) << "," << fit->GetParError(1) << std::endl;

    // Save plot
    // c->SaveAs(Form("%sdata/plots/pulse-heights/pulse_height_npe_run_%d_%.3fGeV.pdf", source_dir.c_str(), run, energy_input));
}

/* @brief: Plots resolution (rms/mean) of the pulse height for each energy/hodoscipe bins
 *          vs energy for a given run
* @params: RDataFrame (RDF::RNode to be exact) that contains the energy column, 
* @returns: None
*/
void plot_resolution_vs_energy(ROOT::RDF::RNode df) {
    std::vector<double> energy_bins = get_energy_bins(source_dir + ps_energy_file);

    int min_ch_value = 500;
    auto filtered_df = df.Filter([=](float ch) { return ch >= min_ch_value; }, {"ch_15"});

    // Output histogram
    TH1D* h_res = new TH1D("h_resolution", "Resolution (RMS/Mean) from Gaussian Fit;Energy [GeV];#sigma / #mu", 
                           static_cast<int>(energy_bins.size()) - 1, energy_bins.data());
    TH1D* h_adc = new TH1D("h_adc", "ADC integral Mean (energy normalized)) ;Energy [GeV];ADC", 
                           static_cast<int>(energy_bins.size()) - 1, energy_bins.data());

    for (int i = 0; i < energy_bins.size() - 1; ++i) {
        // Print progress
        if (i % 10 == 0) {
            cout << "Fitting bin " << i << " of " << energy_bins.size() - 1 << endl;
        }

        double e_low = energy_bins[i];
        double e_high = energy_bins[i + 1];
        double e_center = 0.5 * (e_low + e_high);

        // Filter dataframe within this energy bin
        auto bin_df = filtered_df.Filter(
            [=](double energy) { return energy >= e_low && energy < e_high; }, {"tiler_e"});

        // Get histogram of ch_15 for this bin
        auto h_bin = bin_df.Histo1D({"h_bin", "", 200, 0, 5000}, "ch_15");

        if (h_bin->GetEntries() < 50) {
            h_res->SetBinContent(i + 1, 0);  // skip if not enough entries
            continue;
        }

        // Fit with Gaussian
        TF1* gaus = new TF1("gaus", "gaus", h_bin->GetMean() - 2*h_bin->GetRMS(), h_bin->GetMean() + 2*h_bin->GetRMS());
        h_bin->Fit(gaus, "QR"); // Quiet, use Range

        double mean = gaus->GetParameter(1);
        double sigma = gaus->GetParameter(2);
        double res = sigma / mean;

        if (mean > 0 && res < 0.5) {
            double err_mean = gaus->GetParError(1);
            double err_sigma = gaus->GetParError(2);

            double relative_err = std::sqrt( std::pow(err_sigma / sigma, 2) + std::pow(err_mean / mean, 2));
            double res_error = res * relative_err;

            h_res->SetBinContent(i + 1, res);
            h_res->SetBinError(i + 1, res_error);

            h_adc->SetBinContent(i + 1, mean/e_center);
            h_adc->SetBinError(i + 1, err_mean);
        } else {
            h_res->SetBinContent(i + 1, 0);
            h_res->SetBinError(i + 1, 0);

            h_adc->SetBinContent(i + 1, 0);
            h_adc->SetBinError(i + 1, 0);
        }

        delete gaus;
    }

    // Draw the resolution plot
    TCanvas *c = new TCanvas("c_resolution", "Resolution from Gaussian Fit", 1500, 600);
    c->Divide(2, 1);

    c->cd(1);
    gPad->SetGrid();
    h_adc->Draw("hist E");

    c->cd(2);
    gPad->SetGrid();
    h_res->Draw("hist E");

    c->Update();
}

/* @brief: Plots resolution (rms/mean) of the pulse height for each energy/hodoscipe bins
 *          vs energy for a given run
* @params: RDataFrame (RDF::RNode to be exact) that contains the energy column, 
* @returns: None
*/
void plot_meanADC_vs_energy(ROOT::RDF::RNode df) {
    std::vector<double> energy_bins = get_energy_bins(source_dir + ps_energy_file);

    int min_ch_value = 500;
    auto filtered_df = df.Filter([=](float ch) { return ch >= min_ch_value; }, {"ch_15"});

    // Output histogram
    TH1D* h_adc = new TH1D("h_adc", "ADC integral ;Energy [GeV];ADC", 
                           static_cast<int>(energy_bins.size()) - 1, energy_bins.data());

    for (int i = 0; i < energy_bins.size() - 1; ++i) {
        // Print progress
        if (i % 10 == 0) {
            cout << "Fitting bin " << i << " of " << energy_bins.size() - 1 << endl;
        }

        double e_low = energy_bins[i];
        double e_high = energy_bins[i + 1];
        double e_center = 0.5 * (e_low + e_high);

        // Filter dataframe within this energy bin
        auto bin_df = filtered_df.Filter(
            [=](double energy) { return energy >= e_low && energy < e_high; }, {"tiler_e"});

        // Get histogram of ch_15 for this bin
        auto h_bin = bin_df.Histo1D({"h_bin", "", 200, 0, 5000}, "ch_15");

        if (h_bin->GetEntries() < 50) {
            h_adc->SetBinContent(i + 1, 0);  // skip if not enough e#muntries
            continue;
        }

        // Fit with Gaussian
        TF1* gaus = new TF1("gaus", "gaus", h_bin->GetMean() - 2*h_bin->GetRMS(), h_bin->GetMean() + 2*h_bin->GetRMS());
        h_bin->Fit(gaus, "QR"); // Quiet, use Range

        double mean = gaus->GetParameter(1);

        if (mean > 0) {
            double mean_err = gaus->GetParError(1);

            h_adc->SetBinContent(i + 1, mean);
            h_adc->SetBinError(i + 1, mean_err);
        } else {
            h_adc->SetBinContent(i + 1, 0);
            h_adc->SetBinError(i + 1, 0);
        }

        delete gaus;
    }

    // Draw the resolution plot
    TCanvas *c = new TCanvas("c_resolution", "Resolution from Gaussian Fit", 800, 600);
    gPad->SetGrid();
    h_adc->Draw("histe E");
    c->Update();
}

/* @brief: Normalize the integral ADC value by energy value of the respetive energy_bins
 *          and for the bins that Shower-max intercept, fit gaussian and use the mean from fit
 * @params: RDataFrame (RDF::RNode to be exact) that contains the energy column,
 * @returns: None
 */
void plot_energy_normalized_adc(ROOT::RDF::RNode df, int run){
    std::vector<double> energy_bins = get_energy_bins(source_dir + ps_energy_file);

    // RunInfo
    auto run_info = get_run_info(run);

    // Create profile: Mean ADC vs Energy to select the bins with signal entries
    auto h_mean = df .Profile1D({"h_mean", "Mean of ch_15 vs Energy;Energy [GeV];Mean ADC", static_cast<int>(energy_bins.size()) - 1, energy_bins.data()}, "tiler_e", "ch_15");
    

    // Final histogram for energy-normalized ADC
    TH1D* h_norm = new TH1D("h_enorm_ADC", Form("Energy Normalized ADC Integral Mean from Gaussian Fit, Run %d, Pos(%.0f, %.0f);Energy [GeV];Mean ADC / E_bin", run, run_info.x_pos, run_info.y_pos), static_cast<int>(energy_bins.size()) - 1, energy_bins.data());
    h_norm->SetAxisRange(0,1000, "Y");

    // Loop over bins and normalize mean ADC by energy bin center
    for (int i = 0; i < energy_bins.size(); i++) {
        if (i % 10 == 0) {
            cout << "Processing bin " << i << " of " << energy_bins.size() - 1 << endl;
        }
        // cout << "Bin min: " << energy_bins[i] << " Bin max: " << energy_bins[i+1] << endl;

        double mean_adc = h_mean->GetBinContent(i);
        double mean_error = h_mean->GetBinError(i);

        // Calculate bin center
         double e_center = (energy_bins[i] + energy_bins[i+1]) / 2.0;

        if (mean_adc > 100) {
            // Filter dataframe within this energy bin
            auto df_bin = df.Filter([=](double e) { return e >= energy_bins[i] && e < energy_bins[i+1]; }, {"tiler_e"});
            // Make histogram of ADC for this bin
            auto h_adc = df_bin.Histo1D({"h_mean", "", 100, 300, 5000}, "ch_15");

            // Gaussian fit
            TF1* gaus = new TF1("gaus", "gaus", h_adc->GetMean() - 3*h_adc->GetRMS(), h_adc->GetMean() + 3*h_adc->GetRMS());
            h_adc->Fit(gaus, "QR0 goff"); // Quiet, use Range

            // h_norm->SetBinContent(i, h_adc->GetMean() / e_center);
            // h_norm->SetBinError(i, h_adc->GetMeanError() / e_center); // Need further propogation of error
            h_norm->SetBinContent(i, gaus->GetParameter(1) / e_center);
            h_norm->SetBinError(i, gaus->GetParError(1) / e_center);
        } else {
            h_norm->SetBinContent(i, 0);
            h_norm->SetBinError(i, 0);
        }
    }

    // Linear fitting: find first bin where mean > threshold
    // double threshold = (run<=1118) ? 500 : 2000;
    double threshold = run_info.adc_amp_min_fit;

    const int edge_offset = 0;
    int profFitStartBin;
    int profFitEndBin;
    double fitx1, fitx2;
    int firstBinAboveThreshold = -1;
    int lastBinAboveThreshold = -1;
    int nBins = h_norm->GetNbinsX();

    for (int i = 1; i <= nBins; ++i) {
        if (h_norm->GetBinContent(i) > threshold) {
            firstBinAboveThreshold = i;
            break;
        }
    }

    if (firstBinAboveThreshold == -1) {
        std::cerr << "Warning: No bin with mean > threshold. Skipping fit." << std::endl;
    } else {
        // Loop backward from the last bin to find where mean value starts above threshold
        for (int i = nBins; i <= nBins; i--) {
            if (h_norm->GetBinContent(i) > threshold) {
                lastBinAboveThreshold = i;
                break;
            }
        }

        profFitStartBin = firstBinAboveThreshold + edge_offset;
        profFitEndBin = lastBinAboveThreshold - edge_offset;

        fitx1 = h_norm->GetBinLowEdge(profFitStartBin);
        fitx2 = h_norm->GetBinLowEdge(profFitEndBin - 1);

    }

    // Fit with Gaussian
    TF1* fit = new TF1("pol1", "pol1", fitx1, fitx2);

    // Draw the result
    TCanvas* c = new TCanvas("c_energy_norm", "ADC Normalized by Energy Bin Center", 800, 600);
    gPad->SetGrid();
    gStyle->SetOptStat("e");
    gStyle->SetOptFit(1);
    h_norm->Draw("E");
    h_norm->Fit(fit, "RQ");
    c->Update();

    // Place statbox to bottom right
    TPaveStats* stats = (TPaveStats*)h_norm->FindObject("stats");
    if (stats){
        stats->SetX1NDC(0.5);
        stats->SetX2NDC(0.9);
        stats->SetY1NDC(0.1);
        stats->SetY2NDC(0.3);
    }

    // gPad->Update();
    c->Modified();

    // Save the canvas
    c->SaveAs(Form("%sdata/plots/enorm-adc/plot_energy_normalized_adc_gausfit_run_%d.pdf", source_dir.c_str(), run));

    // Open the output csv file and rerwrite the fit parameters
    std::ofstream output_file(Form("%sdata/outfiles/enorm_fit_params.csv", source_dir.c_str()), std::ios::app);
    output_file << run << "," << run_info.x_pos << "," << run_info.y_pos << "," <<
        fit->GetParameter(0) << "," << fit->GetParError(0) << "," << 
        fit->GetParameter(1) << "," << fit->GetParError(1) << std::endl;

    c->Update();
}

/*
* @brief: Plot the raw wavefrom which are stored as TProfiles in a ROOT file
* @params: name of the ROOT file that contains the TProfiles waveforms
* @return: None
*/
void plot_waveform_profiles(const std::string& waveform_rootfile) {
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
            prof->SetLineColor(kBlue);
            prof->SetTitle(Form("Waveform: %d;Sample [4ns];SumADC/4ns", idx + 1));
            prof->Draw("hist");
        }

        canvas->Update();

        // Save the canvas as a PDF
        canvas->SaveAs(Form("%sdata/plots/waveforms/waveform_profiles_%d.pdf", source_dir.c_str(), c+1));
    }
    gSystem->Exec(Form("pdfunite %sdata/plots/waveforms/waveform_profiles_*.pdf %sdata/plots/waveforms/waveform_profiles.pdf", source_dir.c_str(), source_dir.c_str()));
    gSystem->Exec(Form("rm %sdata/plots/waveforms/waveform_profiles_*.pdf", source_dir.c_str()));
}

/*
* @brief: Plot the graphs of the waveforms saved as arrays in the rootfile
* @params: rootfile name (not the full path)
* @return: None
*/
void plot_waveform_arrays(const std::string& rootfile) {
    gStyle->SetOptStat("neMR");
    gStyle->SetOptFit(1);
    string file = rootfile;

    TFile* f = TFile::Open(file.c_str(), "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Error opening file: " << file << std::endl;
        return;
    }

    TTreeReader reader("tree1", f);
    TTreeReaderArray<Float_t> waveform(reader, "Waveform");

    // TGraph of Waveforms
    vector<TGraph*> graphs;

    while (reader.Next()) {
        bool has_signal_peak = false;
        bool has_huge_peak = false; // not a single pe peak; tq signal

        // Check for signal peak in waveform
        for (int i = 0; i < waveform.GetSize(); ++i) {
            float adc = waveform[i];
            if (adc > 330) {
                has_huge_peak = true;
                break;
            }
            if (adc > 105) {
                has_signal_peak = true;
            }
        }

        if (has_huge_peak) { continue; }

        if (has_signal_peak) {
            // Only create and store the graph if it's a signal
            TGraph* graph = new TGraph();
            for (int iX = 0; iX < waveform.GetSize(); ++iX) {
                graph->SetPoint(iX, iX, waveform[iX]);
            }
            graph->SetTitle(Form("Waveform: %lld;Sample [4ns];SumADC/4ns", reader.GetCurrentEntry()));
            graphs.push_back(graph);
        }
    }

    // Plot 100 waveorms in 5 canvases
    // for (int i = 1; i < graphs.size()/20; i++) {
    for (int i = 1; i < 2; i++) {
        TCanvas* c_waveform = new TCanvas(Form("c_waveform_%d", i), Form("Waveform %d", i), 1600, 1200);
        c_waveform->Divide(5,4);
        for (int j = 1; j <= 20; j++) {
            c_waveform->cd(j);
            graphs[i*20+j]->SetLineColor(kBlue + 1);
            graphs[i*20+j]->SetMarkerStyle(20);
            graphs[i*20+j]->SetMarkerSize(0.5);
            // graphs[i*20+j]->SetFillColorAlpha(kBlue, 0.5); // semi-transparent fill
            graphs[i*20+j]->Draw("AL"); // 'A' = axis, 'B' = bar style

        }

        // Save canvas as pdf and merge pdfs
        c_waveform->SaveAs(Form("%sdata/plots/waveforms/waveform_lightguide_%03d.pdf", source_dir.c_str(), i));
    }
    // gSystem->Exec(Form("pdfunite %sdata/plots/waveforms/waveform_lightguide_*.pdf %sdata/plots/waveforms/waveform_lightguide.pdf", source_dir.c_str(), source_dir.c_str()));
    // gSystem->Exec(Form("rm %sdata/plots/waveforms/waveform_lightguide_*.pdf", source_dir.c_str()));
}

/*
* @brief: Plot the histogram of raw AC vaues (amplitude)
*           Side by side comparison of beam hitting at the LG and TQ
* @params: None
* @return: None
*/
void plot_gain_calibration_raw_amp(){
    string file1 = source_dir + gain_calibration_rootfile_lg;
    string file2 = source_dir + gain_calibration_rootfile_tq;

    TCanvas* c_raw = new TCanvas("c_raw", "Gain Calibration Run", 1200, 500);
    c_raw->Divide(2, 1);

    TFile* fileLG = TFile::Open(file1.c_str(), "READ");
    if (!fileLG || fileLG->IsZombie()) {
        std::cerr << "Error: Cannot open file " << file1 << std::endl;
        return;
    }

    TFile* fileTQ = TFile::Open(file2.c_str(), "READ");
    if (!fileTQ || fileTQ->IsZombie()) {
        std::cerr << "Error: Cannot open file " << file2 << std::endl;
        return;
    }

    TTree* treeLG = (TTree*)fileLG->Get("tree1");
    TTree* treeTQ = (TTree*)fileTQ->Get("tree1");

    if (!treeLG || !treeTQ) {
        std::cerr << "Error: Tree not found in one or both files." << std::endl;
        return;
    }

    // treeLG->Print(); // Debug
    // treeTQ->Print();

    TH1F *histLG = new TH1F("histLG", "Wavefrom amplitue at light guide;ADC amplitude;Counts", 1000, 0, 1000);
    TH1F *histTQ = new TH1F("histTQ", "Wavefrom amplitue at center at tungsten-quartz;ADC amplitude;Counts", 1000, 0, 1000);

    c_raw->cd(1);
    gPad->SetGrid();
    gPad->SetLogy();
    treeLG->Draw("Waveform>>histLG");
    histLG->Draw("hist E");

    c_raw->cd(2);
    gPad->SetGrid();
    gPad->SetLogy();
    treeTQ->Draw("Waveform>>histTQ");
    histTQ->Draw("hist E");
}

/*
* @brief: Plot the histogram of raw ADC vaues (amplitude)
*           Side by side comparison of beam hitting at the LG and TQ
* @params: None
* @return: None
*/
void plot_gain_calibration_single_pe(){
    gStyle->SetOptStat("neMR");
    gStyle->SetOptFit(1);
    string file = source_dir + gain_calibration_rootfile_lg;

    TFile* f = TFile::Open(file.c_str(), "READ");
    if (!f || f->IsZombie()) {
        std::cerr << "Error opening file: " << file << std::endl;
        return;
    }

    TTreeReader reader("tree1", f);
    TTreeReaderArray<Float_t> waveform(reader, "Waveform");

    // Histograms 
    TH1F* hist_pedsub = new TH1F("hist_pedsub", "Signal Waveforms;Sum;Entries", 1000, 7500, 8500);
    TH1F* hist_signal = new TH1F("hist_signal", "Signal Waveforms;Sum;Entries", 1000, 7500, 8500);
    TH1F* hist_ped    = new TH1F("hist_ped",    "Pedestal Waveforms;Sum;Entries", 1000, 7500, 8500);
    TH1F* h_mean = new TH1F("h_mean", "Waveform Mean;Mean ADC;Entries", 100, 80, 120);
    TH1F* h_rms  = new TH1F("h_rms", "Waveform RMS;RMS;Entries", 100, 0, 20);
    TH1F* h_ratio = new TH1F("h_ratio", "RMS / Mean;RMS/Mean;Entries", 100, 0, 0.5);

    // TGraph of Waveforms
    vector<TGraph*> graphs;

    while (reader.Next()) {
        bool has_signal = false;
        float sum1 = 0.0;

        // Check for signal peak in waveform
        for (int i = 0; i < waveform.GetSize(); ++i) {
            sum1 += waveform[i];
            if (waveform[i] > 105) {
                has_signal = true;
            }
        }


        float integral = sum1;

        if (has_signal) {
            // Only create and store the graph if it's a signal
            TGraph* graph = new TGraph();
            for (int iX = 0; iX < waveform.GetSize(); ++iX) {
                graph->SetPoint(iX, iX, waveform[iX]);
            }
            graph->SetTitle(Form("Waveform: %lld;time[ns];SumADC/4ns", reader.GetCurrentEntry()));
            graphs.push_back(graph);

            hist_signal->Fill(integral);
        } else {
            hist_ped->Fill(integral);
        }

        // Calculate mean and rms
        int n = waveform.GetSize();
        float sum = 0;
        float sum2 = 0;

        for (int i = 0; i < n; ++i) {
            sum  += waveform[i];
            sum2 += waveform[i] * waveform[i];
        }

        // float mean = sum / n;
        // float rms = std::sqrt(sum2 / n - mean * mean);
        // float ratio = (mean != 0) ? rms / mean : 0;
        //
        // h_mean->Fill(mean);
        // h_rms->Fill(rms);
        // h_ratio->Fill(ratio);
    }

    // TCanvas *c_gain = new TCanvas("c_gain", "Gain Calibration Run", 800, 600);
    // gPad->SetGrid();

    // Fit 
    // TF1 *gaus_fit = new TF1("gaus_fit", "gaus", 8000, 8500);
    // hist_int->Fit(gaus_fit, "RQ");
    // gaus_fit->SetLineColor(kRed);

    // hist_int->Draw("hist E");
    // gaus_fit->Draw("same");
    // hist_ped->Draw("hist E");
    // hist_signal->Draw("hist E same");

    // Setup canvas with 3 pads
    // TCanvas* c_gain2 = new TCanvas("c_gain2", "Waveform Stats", 1200, 400);
    // c_gain2->Divide(3,1); // 3 pads horizontally
    //
    // c_gain2->cd(1);
    // h_mean->SetLineColor(kBlue);
    // h_mean->Draw();
    //
    // c_gain2->cd(2);
    // h_rms->SetLineColor(kRed);
    // h_rms->Draw();
    //
    // c_gain2->cd(3);
    // h_ratio->SetLineColor(kGreen+2);
    // h_ratio->Draw();

    // c_gain->Update();

    // Plot 100 waveorms in 5 canvases
    // for (int i = 1; i < graphs.size()/20; i++) {
    for (int i = 1; i < 2; i++) {
        TCanvas* c_waveform = new TCanvas(Form("c_waveform_%d", i), Form("Waveform %d", i), 1600, 1200);
        c_waveform->Divide(5,4);
        for (int j = 1; j <= 20; j++) {
            c_waveform->cd(j);
            graphs[i*20+j]->Draw("AL");
        }

        // Save canvas as pdf and merge pdfs
        c_waveform->SaveAs(Form("%sdata/plots/waveforms/waveform_lightguide_%03d.pdf", source_dir.c_str(), i));
    }
    // gSystem->Exec(Form("pdfunite %sdata/plots/waveforms/waveform_lightguide_*.pdf %sdata/plots/waveforms/waveform_lightguide.pdf", source_dir.c_str(), source_dir.c_str()));
    // gSystem->Exec(Form("rm %sdata/plots/waveforms/waveform_lightguide_*.pdf", source_dir.c_str()));

}

/* @brief: Compare the simulation and testbeam data pulse height distribution
* @params: 1: RDataFrame (RDF::RNode to be exact) that contains the energy column,
*           2: the local run number (1XXX), not halld run number
*           3: the beam energy to compare (3.0, 4.5 or 6.0)
* @returns: None
*/
void plot_compare_sim_testbeam(string sim_rootfile, ROOT::RDF::RNode df, int target_run, double beamEnergy) {
    // Set style
    gStyle->SetOptStat("neMRi");

    // File path
    string file_path = source_dir + sim_rootfile;


    // Check if the file exists
    if (!TFile::Open(sim_rootfile.c_str(), "READ")) {
        std::cerr << "File " << sim_rootfile << " does not exist. Exiting..." << std::endl;
        return;
    }

    // Open simulation ROOT file
    TFile *file_sim = TFile::Open(sim_rootfile.c_str(), "READ");
    if (!file_sim || file_sim->IsZombie()) {
        std::cerr << "Failed to open file or corrupted: " << sim_rootfile << std::endl;
        return;
    }

    // Get simulation histogram
    std::string histname = Form("h_hitn_%.1fGeV;1", beamEnergy);
    TH1D *hist_sim = static_cast<TH1D *>(file_sim->Get(histname.c_str()));
    if (!hist_sim) {
        std::cerr << "Histogram " << histname << " not found in " << sim_rootfile << std::endl;
        file_sim->Close();
        exit(1);
    }
    hist_sim->SetName("simulation");
    hist_sim->SetLineColor(kRed);
    hist_sim->SetLineWidth(2);
    hist_sim->SetTitle(Form("Sim vs Testbeam Pulse Height at %.3f GeV", beamEnergy));

    // Energy window cut
    double energy_window = 0.026;  // +/-26 MeV
    double e_min = beamEnergy - energy_window;
    double e_max = beamEnergy + energy_window;

    auto df_eFilter = df.Filter([=](double e) { return e >= e_min && e <= e_max; }, {"tiler_e"});
    double boost_factor = 5.0; // Match the simulation 
    auto df_boost = df_eFilter.Define("tiler_npe_boost", [=](double npe) { return npe * boost_factor; }, {"tiler_npe"});

    // Create testbeam histogram from DataFrame
    auto hist_testbeam = df_boost.Histo1D(
        {"hist_testbeam_nPEs",
            Form("Sim vs Testbeam Pulse Height at %.3f #pm 0.026 GeV;Number of PEs;Counts", beamEnergy),
            500, 1, 501},
        "tiler_npe_boost");

    hist_testbeam->SetLineColor(kGreen + 2);
    hist_testbeam->SetLineWidth(2);

    // Fit testbeam histogram
    // int peak_bin = hist_testbeam->GetMaximumBin();
    // double peak_center = hist_testbeam->GetBinCenter(peak_bin);
    // double fit_min = std::max(1.0, peak_center - 20);
    // double fit_max = std::min(500.0, peak_center + 30);
    // hist_testbeam->Fit("gaus", "", "", fit_min, fit_max);

    // Canvas for plot
    TCanvas *c = new TCanvas("c_compare", "Sim vs Testbeam Pulse Height", 800, 600);
    c->cd();

    // Clone testbeam histogram
    auto hist_testbeam_clone = static_cast<TH1D *>(hist_testbeam->Clone("testbeam x 5"));
    hist_testbeam_clone->SetLineColor(kBlue + 2);
    hist_testbeam_clone->SetStats(1);
    hist_sim->SetLineColor(kRed);
    hist_sim->SetStats(1);

    // normalize Histograms
    hist_testbeam_clone->Scale(1.0 / hist_testbeam_clone->Integral());
    hist_sim->Scale(1.0 / hist_sim->Integral());

    // Rebin histograms
    hist_testbeam_clone->Rebin(5);
    hist_sim->Rebin(5);

    // Draw testbeam first to generate stats
    hist_sim->Draw("hist e");
    hist_testbeam_clone->Draw("hist e sames");

    gPad->Update();  // Stat boxes are now created

    // Array of histograms
    const int nHists = 2;
    TH1D* h_hitn[nHists] = {hist_testbeam_clone, hist_sim};
    TPaveStats* stat[nHists];

    float widthStat = 0.35;
    float heightStat = 0.18;
    float heightgap = 0.05;
    int nColStat = 1;

    for (int i = 0; i < nHists; i++) {
        // Access the stat box from the histogram itself
        stat[i] = (TPaveStats*)h_hitn[i]->FindObject("stats");
        if (!stat[i]) {
            std::cerr << "Stat box not found for histogram " << h_hitn[i]->GetName() << std::endl;
            continue;
        };

        stat[i]->SetTextColor(h_hitn[i]->GetLineColor());
        stat[i]->SetX1NDC(0.9 - widthStat - (i % nColStat) * widthStat);
        stat[i]->SetX2NDC(0.9 - (i % nColStat) * widthStat);
        stat[i]->SetY1NDC(0.9 - heightStat - (float(i) / nColStat) * heightStat - i*heightgap);
        stat[i]->SetY2NDC(0.9 - (float(i) / nColStat) * heightStat - i*heightgap);

        // Add RMS/Mean line
        TList *listOfLines = stat[i]->GetListOfLines();
        double res = h_hitn[i]->GetRMS() / h_hitn[i]->GetMean();
        cout << res << endl;
        TText *texres = new TText(0, 0, Form("RMS/Mean = %.2f", res));
        texres->SetTextFont(42);
        texres->SetTextSize(0.03);
        texres->SetTextColor(h_hitn[i]->GetLineColor());
        stat[i]->GetListOfLines()->Add(texres);

        // Draw text
        texres->DrawTextNDC(0.9 - widthStat - (i % nColStat) * widthStat, 0.9 - (float(i+1) / nColStat) * heightStat - (i+1)*heightgap+0.01, Form("RMS/Mean = %.3f", res));


        // h_hitn[i]->SetStats(0);
        // gPad->Modified();
    }
    // Save canvas
    c->SaveAs(Form("%sdata/plots/plot_compare_sim_testbeam_%.3fGeV.pdf", source_dir.c_str(), beamEnergy));
}

/* @brief: Compare the simulation with LP filter response and testbeam data pulse height distribution
* @params: 1: RDataFrame (RDF::RNode to be exact) that contains the energy column,
*           2: the local run number (1XXX), not halld run number
*           3: the beam energy to compare (3.0, 4.5 or 6.0)
* @returns: None
*/
void plot_compare_simLP_testbeam(string sim_rootfile, ROOT::RDF::RNode df, int target_run, double beamEnergy) {
    // Set style
    gStyle->SetOptStat("neMRi");

    // File path
    string file_path = source_dir + sim_rootfile;


    // Check if the file exists
    if (!TFile::Open(sim_rootfile.c_str(), "READ")) {
        std::cerr << "File " << sim_rootfile << " does not exist. Exiting..." << std::endl;
        return;
    }

    // Open simulation ROOT file
    TFile *file_sim = TFile::Open(sim_rootfile.c_str(), "READ");
    if (!file_sim || file_sim->IsZombie()) {
        std::cerr << "Failed to open file or corrupted: " << sim_rootfile << std::endl;
        return;
    }

    // Get simulation histogram
    std::string histname = Form("h_hitn_%.1fGeV_lp450;1", beamEnergy);
    TH1D *hist_sim = static_cast<TH1D *>(file_sim->Get(histname.c_str()));
    if (!hist_sim) {
        std::cerr << "Histogram " << histname << " not found in " << sim_rootfile << std::endl;
        file_sim->Close();
        exit(1);
    }
    hist_sim->SetName("simulation: LP 450 nm");
    hist_sim->SetLineColor(kRed);
    hist_sim->SetLineWidth(2);
    hist_sim->SetTitle(Form("Sim vs Testbeam Pulse Height at %.3f GeV", beamEnergy));

    // Energy window cut
    double energy_window = 0.026;  // +/-26 MeV
    double e_min = beamEnergy - energy_window;
    double e_max = beamEnergy + energy_window;

    auto df_eFilter = df.Filter([=](double e) { return e >= e_min && e <= e_max; }, {"tiler_e"});
    double boost_factor = 1.0; // Match the simulation to 
    auto df_boost = df_eFilter.Define("tiler_npe_boost", [=](double npe) { return npe * boost_factor; }, {"tiler_npe"});

    // Create testbeam histogram from DataFrame
    auto hist_testbeam = df_boost.Histo1D(
        {"hist_testbeam_nPEs",
            Form("Sim vs Testbeam Pulse Height at %.3f #pm 0.026 GeV;Number of PEs;Counts/Entries/1PEbin", beamEnergy),
            100, 1, 101},
        "tiler_npe_boost");

    hist_testbeam->SetLineColor(kGreen + 2);
    hist_testbeam->SetLineWidth(2);

    // Fit testbeam histogram
    // int peak_bin = hist_testbeam->GetMaximumBin();
    // double peak_center = hist_testbeam->GetBinCenter(peak_bin);
    // double fit_min = std::max(1.0, peak_center - 20);
    // double fit_max = std::min(500.0, peak_center + 30);
    // hist_testbeam->Fit("gaus", "", "", fit_min, fit_max);

    // Canvas for plot
    TCanvas *c = new TCanvas("c_simlp_testbeam", "Sim vs Testbeam Pulse Height", 800, 600);
    c->cd();

    // Clone testbeam histogram
    auto hist_testbeam_clone = static_cast<TH1D *>(hist_testbeam->Clone("testbeam"));
    hist_testbeam_clone->SetLineColor(kBlue + 2);
    hist_testbeam_clone->SetStats(1);
    hist_sim->SetLineColor(kRed);
    hist_sim->SetStats(1);

    // normalize Histograms
    hist_testbeam_clone->Scale(1.0 / hist_testbeam_clone->Integral());
    hist_sim->Scale(1.0 / hist_sim->Integral());

    // Rebin histograms
    int rebin_factor = 1;
    hist_testbeam_clone->Rebin(rebin_factor);
    hist_sim->Rebin(rebin_factor);

    // Draw testbeam first to generate stats
    hist_testbeam_clone->Draw("hist e");
    hist_sim->Draw("hist e sames");

    gPad->Update();  // Stat boxes are now created

    // Array of histograms
    const int nHists = 2;
    TH1D* h_hitn[nHists] = {hist_testbeam_clone, hist_sim};
    TPaveStats* stat[nHists];

    float widthStat = 0.35;
    float heightStat = 0.18;
    float heightgap = 0.05;
    int nColStat = 1;

    for (int i = 0; i < nHists; i++) {
        // Access the stat box from the histogram itself
        stat[i] = (TPaveStats*)h_hitn[i]->FindObject("stats");
        if (!stat[i]) {
            std::cerr << "Stat box not found for histogram " << h_hitn[i]->GetName() << std::endl;
            continue;
        };

        stat[i]->SetTextColor(h_hitn[i]->GetLineColor());
        stat[i]->SetX1NDC(0.9 - widthStat - (i % nColStat) * widthStat);
        stat[i]->SetX2NDC(0.9 - (i % nColStat) * widthStat);
        stat[i]->SetY1NDC(0.9 - heightStat - (float(i) / nColStat) * heightStat - i*heightgap);
        stat[i]->SetY2NDC(0.9 - (float(i) / nColStat) * heightStat - i*heightgap);

        // Add RMS/Mean line
        TList *listOfLines = stat[i]->GetListOfLines();
        double res = h_hitn[i]->GetRMS() / h_hitn[i]->GetMean();
        cout << res << endl;
        TText *texres = new TText(0, 0, Form("RMS/Mean = %.2f", res));
        texres->SetTextFont(42);
        texres->SetTextSize(0.03);
        texres->SetTextColor(h_hitn[i]->GetLineColor());
        stat[i]->GetListOfLines()->Add(texres);

        // Draw text
        texres->DrawTextNDC(0.9 - widthStat - (i % nColStat) * widthStat, 0.9 - (float(i+1) / nColStat) * heightStat - (i+1)*heightgap+0.01, Form("RMS/Mean = %.3f", res));

    }
    // Save canvas
    c->SaveAs(Form("%sdata/plots/plot_compare_simLP450nm_testbeam_%.3fGeV.pdf", source_dir.c_str(), beamEnergy));
}


/* @brief: From the energy normalized TProfile plots the slope is written to the csv files
 *          This function reads the csv file and plots the slope vs position
 * @params: None
 * @return: None, plot is saved in the ../data/plots/ directory
*/
void plot_energy_normalized_response_slope(){

    std::ifstream infile(enorm_fit_params.c_str());
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open file " << enorm_fit_params << std::endl;
        return;
    }

    std::vector<double> x_vals;
    std::vector<double> y_vals;
    std::vector<double> x_errs;
    std::vector<double> y_errs;

    std::string line;
    bool is_header = true;
    while (std::getline(infile, line)) {
        if (is_header) {
            is_header = false;
            continue;  // skip the header line
        }

        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> tokens;
        while (std::getline(ss, field, ',')) {
            tokens.push_back(field);
        }

        if (tokens.size() < 7) continue;

        double x_pos = std::stod(tokens[1]);
        double p1 = std::stod(tokens[5]);
        double p1err = std::stod(tokens[6]);

        x_vals.push_back(x_pos);
        y_vals.push_back(p1);
        x_errs.push_back(0);     // No error in x_pos
        y_errs.push_back(p1err);
    }

    infile.close();

    int n = x_vals.size();
    if (n == 0) {
        std::cerr << "No data found in file!" << std::endl;
        return;
    }

    TCanvas* c1 = new TCanvas("c1", "X Position vs P1", 800, 600);
    gPad->SetGrid();
    gStyle->SetOptFit(1);  // Show fit info box

    TGraphErrors* graph = new TGraphErrors(n, x_vals.data(), y_vals.data(), x_errs.data(), y_errs.data());
    graph->SetTitle("X Position vs P1;X Position [mm];P1 Value");
    graph->SetMarkerStyle(20);
    graph->SetMarkerSize(1.2);
    graph->Draw("AP");

    TF1* fitfunc = new TF1("fitfunc", "pol2", -200, 200);
    graph->Fit(fitfunc, "R");

    c1->Update();
}
