# Shower-max halld testbeam analysis

This is a repository for analysis of the Shower-max detector beam testing at HALLD of Jefferson lab. The photon beam hits the Beryllium target and pair produce the electron and positron pairs. The leptons further pass through the PS (Pair Spectrometer) which separates the positrons from the electrons. The positron pass through the hodoscope  scintillator which gives the energy information. The file [PS_energy.txt](./data/PS_energy.txt) contains the energy information from the hodoscope. The Shower-max is placed ~1 m downstream of the hodoscope. So at any position the Shower-max is placed it intercept the range of positron energy. 

## Directory/Files details

- `scripts/`: Contains the analysis scripts, the main analysis script is [analyze_testbeam.C](./scripts/analyze_testbeam.C)
- `data/`: Contains the data Files
    1. rootfile from testings (1 evios run) and production (15 evios run)
    2. plots from differnts runs, the sub directory contains the plots of different categories , for eg. replay plots
    3. PS_energy.txt file contains the energy information from the hodoscope
    4. pmt_gain.csv file contains the pmt gain information for diffrent pmts and HV values
    5. run_settings_log.csv contains the run numbers along with the position and pmt informations
- `preparation`: contains the the documents for halld data taking preparation
- `./run_analyzer` is used to run the analysis script with the run number given, the rootfiles are picked from the rootfiles/production directory
- `./run_analyzer_range` is used to run the analysis script with the range of the run numbers given
- `./run_replay` is used to run the replay script with the root file as the input
- `CMakeLists.txt` is the cmake file for the project which is not required to run the analysis but makes the neovim friendly if one wants to use LSPs.

## How to run the analysis

