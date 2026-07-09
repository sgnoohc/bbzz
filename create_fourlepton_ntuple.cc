// ============================================================================
//  create_fourlepton_ntuple.cc
//
//  Four-lepton (ZZ -> 4l) ntuple producer for the RunIII2024Summer24
//  NanoAODv15 sample
//
//    GluGlutoHHto2B2Zto4L_..._powheg-pythia8  (HH -> bb ZZ -> bb 4l)
//
//  Starting point: https://github.com/sgnoohc/rdfutil  (rdfutil.h toolkit)
//
//  The rdfutil templates ship with Run-2 UL-2018 object IDs.  NanoAODv15
//  (Run 3, 150X) renamed / removed several branches, so the object IDs below
//  are re-implemented for Run 3 2024:
//
//    * Electrons : no Electron_mvaFall17V2* branches. Use Electron_mvaIso_WP90/80
//                  (+ Electron_cutBased for the loose tier). deltaEtaSC/dxy/dz/
//                  sip3d preselection as in HZZ-4l.
//    * Muons     : all needed branches still exist; HZZ-4l style selection.
//    * Jets      : Jet_jetId was removed. Recompute Run-3 "Tight" PF jet ID
//                  from the energy-fraction / multiplicity branches.
//    * MET       : no "MET" collection. Run-3 recommends PuppiMET (stored).
//
//  On top of object selection this builds the ZZ -> 4l candidate
//  (Z1 closest to mZ, Z2 = best remaining OSSF pair) and stores mZ1, mZ2, m4l.
//
//  Usage:
//    ./create_fourlepton_ntuple <filelist.txt> <output.root> [nthreads]
//        filelist.txt : text file, one input NanoAOD path per line
//                       ('#' comments and blank lines are ignored)
// ============================================================================

#include "rdfutil.h"   // toolkit from code/rdfutil (see Makefile -I)

// PDG masses (GeV)
static constexpr float kMZ  = 91.1876f;
static constexpr float kMe  = 0.000511f;
static constexpr float kMmu = 0.105658f;

// ============================================================================
// Electron identification for Run 3 (NanoAODv15)
//   Returns a tiered code per electron:
//     0 : fail
//     1 : loose / veto      (baseline + cutBased>=1)   -> extra-lepton veto
//     2 : analysis (tight)  (+ mvaIso_WP90)            -> GoodElectron
//     3 : + mvaIso_WP80
//     4 : + tightCharge==2 && convVeto && lostHits<=1
//   Baseline preselection (HZZ-4l-like):
//     pt>7, |eta_SC|<2.5, |dxy|<0.5, |dz|<1.0, |sip3d|<4
// ============================================================================
auto elec_run3ID = [](const RVec<float>&         Electron_pt,
                      const RVec<float>&         Electron_eta,
                      const RVec<float>&         Electron_deltaEtaSC,
                      const RVec<float>&         Electron_dxy,
                      const RVec<float>&         Electron_dz,
                      const RVec<float>&         Electron_sip3d,
                      const RVec<unsigned char>& Electron_cutBased,
                      const RVec<bool>&          Electron_mvaIso_WP90,
                      const RVec<bool>&          Electron_mvaIso_WP80,
                      const RVec<bool>&          Electron_convVeto,
                      const RVec<unsigned char>& Electron_lostHits,
                      const RVec<unsigned char>& Electron_tightCharge)
{
    const auto n = Electron_pt.size();
    RVec<int> pass(n, 0);
    for (size_t i = 0; i < n; ++i)
    {
        const float etaSC = Electron_eta[i] + Electron_deltaEtaSC[i];
        if (not (Electron_pt[i] > 7.0f))           continue;
        if (not (std::fabs(etaSC) < 2.5f))         continue;
        if (not (std::fabs(Electron_dxy[i]) < 0.5f))  continue;
        if (not (std::fabs(Electron_dz[i])  < 1.0f))  continue;
        if (not (std::fabs(Electron_sip3d[i]) < 4.0f)) continue;
        if (not (Electron_cutBased[i] >= 1))       continue;   // >= veto
        pass[i] = 1;
        if (not (Electron_mvaIso_WP90[i]))         continue;
        pass[i] = 2;
        if (not (Electron_mvaIso_WP80[i]))         continue;
        pass[i] = 3;
        if (not (Electron_tightCharge[i] == 2))    continue;
        if (not (Electron_convVeto[i]))            continue;
        if (not (Electron_lostHits[i] <= 1))       continue;
        pass[i] = 4;
    }
    return pass;
};

std::vector<std::string> elec_run3ID_inputs =
{
    "Electron_pt", "Electron_eta", "Electron_deltaEtaSC", "Electron_dxy",
    "Electron_dz", "Electron_sip3d", "Electron_cutBased",
    "Electron_mvaIso_WP90", "Electron_mvaIso_WP80",
    "Electron_convVeto", "Electron_lostHits", "Electron_tightCharge",
};

// ============================================================================
// Muon identification for Run 3 (NanoAODv15)
//   0 : fail
//   1 : loose  (baseline + looseId + iso<0.4)          -> extra-lepton veto
//   2 : analysis (medium + iso<0.25)                   -> GoodMuon
//   3 : + iso<0.15
//   4 : + tightId + iso<0.10
//   Baseline (HZZ-4l-like): pt>5, |eta|<2.4, |dxy|<0.5, |dz|<1.0, |sip3d|<4
// ============================================================================
auto muon_run3ID = [](const RVec<float>& Muon_pt,
                      const RVec<float>& Muon_eta,
                      const RVec<float>& Muon_dxy,
                      const RVec<float>& Muon_dz,
                      const RVec<float>& Muon_sip3d,
                      const RVec<float>& Muon_pfRelIso04_all,
                      const RVec<bool>&  Muon_looseId,
                      const RVec<bool>&  Muon_mediumId,
                      const RVec<bool>&  Muon_tightId)
{
    const auto n = Muon_pt.size();
    RVec<int> pass(n, 0);
    for (size_t i = 0; i < n; ++i)
    {
        if (not (Muon_pt[i] > 5.0f))               continue;
        if (not (std::fabs(Muon_eta[i]) < 2.4f))   continue;
        if (not (std::fabs(Muon_dxy[i]) < 0.5f))   continue;
        if (not (std::fabs(Muon_dz[i])  < 1.0f))   continue;
        if (not (std::fabs(Muon_sip3d[i]) < 4.0f)) continue;
        if (not (Muon_looseId[i]))                 continue;
        if (not (Muon_pfRelIso04_all[i] < 0.4f))   continue;
        pass[i] = 1;
        if (not (Muon_mediumId[i]))                continue;
        if (not (Muon_pfRelIso04_all[i] < 0.25f))  continue;
        pass[i] = 2;
        if (not (Muon_pfRelIso04_all[i] < 0.15f))  continue;
        pass[i] = 3;
        if (not (Muon_tightId[i]))                 continue;
        if (not (Muon_pfRelIso04_all[i] < 0.10f))  continue;
        pass[i] = 4;
    }
    return pass;
};

std::vector<std::string> muon_run3ID_inputs =
{
    "Muon_pt", "Muon_eta", "Muon_dxy", "Muon_dz", "Muon_sip3d",
    "Muon_pfRelIso04_all", "Muon_looseId", "Muon_mediumId", "Muon_tightId",
};

// ============================================================================
// Jet identification for Run 3 (NanoAODv15)
//   Jet_jetId was removed in NanoAODv15, so recompute the Run-3 "Tight" PF
//   jet ID from the energy fractions / multiplicities.  Central recipe
//   (|eta| <= 2.6); this workflow only keeps |eta| < 2.5 jets.
//     Tight (|eta|<=2.6): NHF<0.99 && NEMF<0.90 && nConstituents>1
//                         && CHF>0.01 && CHM>0
//   Kinematics: pt >= 20, |eta| < 2.5
// ============================================================================
auto jet_run3ID = [](const RVec<float>&         Jet_pt,
                     const RVec<float>&         Jet_eta,
                     const RVec<float>&         Jet_neHEF,
                     const RVec<float>&         Jet_neEmEF,
                     const RVec<float>&         Jet_chHEF,
                     const RVec<unsigned char>& Jet_chMultiplicity,
                     const RVec<unsigned char>& Jet_nConstituents)
{
    const auto n = Jet_pt.size();
    RVec<bool> pass(n, false);
    for (size_t j = 0; j < n; ++j)
    {
        if (not (Jet_pt[j] >= 20.0f))              continue;
        if (not (std::fabs(Jet_eta[j]) < 2.5f))    continue;
        // Run-3 Tight PF jet ID (central region)
        if (not (Jet_neHEF[j]  < 0.99f))           continue;
        if (not (Jet_neEmEF[j] < 0.90f))           continue;
        if (not (Jet_nConstituents[j] > 1))        continue;
        if (not (Jet_chHEF[j]  > 0.01f))           continue;
        if (not (Jet_chMultiplicity[j] > 0))       continue;
        pass[j] = true;
    }
    return pass;
};

std::vector<std::string> jet_run3ID_inputs =
{
    "Jet_pt", "Jet_eta", "Jet_neHEF", "Jet_neEmEF", "Jet_chHEF",
    "Jet_chMultiplicity", "Jet_nConstituents",
};

// Branches copied into the trimmed "Good" collections
std::vector<std::string> AnaMuon_props     = {"pt", "eta", "phi", "charge"};
std::vector<std::string> AnaElectron_props = {"pt", "eta", "phi", "charge"};
std::vector<std::string> AnaJet_props      = {"pt", "eta", "phi", "mass",
                                              "btagDeepFlavB", "btagPNetB"};

// ============================================================================
// ZZ -> 4l reconstruction
//   Input : a merged GoodLepton collection (pt/eta/phi/charge/pdgId), sorted
//           by descending pt (as produced by RdfUtil::MergeCollections).
//   Z1 = OSSF pair with mass closest to mZ
//   Z2 = best remaining OSSF pair (largest scalar pt sum)
//   Fills mZ1, mZ2, m4l, the ZZ-system kinematics, the 4 lepton indices, and
//   a channel code (0=4mu, 1=4e, 2=2e2mu).  ok=0 if no valid 2+2 OSSF pairing.
// ============================================================================
struct ZZCand
{
    int   ok;
    int   channel;                       // 0=4mu 1=4e 2=2e2mu, -1 invalid
    float mZ1, mZ2, m4l;
    float Z1pt, Z2pt, pt4l, eta4l, phi4l;
    int   l1, l2, l3, l4;                // indices into GoodLepton
};

static RNode BuildZZ4l(RNode df, const std::string& p, const std::string& out)
{
    const std::string carrier = "__" + out + "_zzCarrier";

    df = df.Define(carrier,
        [](const RVec<float>& pt, const RVec<float>& eta, const RVec<float>& phi,
           const RVec<int>& /*charge*/, const RVec<int>& pdgId) -> ZZCand
        {
            ZZCand c;
            c.ok = 0; c.channel = -1;
            c.mZ1 = c.mZ2 = c.m4l = -1.f;
            c.Z1pt = c.Z2pt = c.pt4l = c.eta4l = c.phi4l = -1.f;
            c.l1 = c.l2 = c.l3 = c.l4 = -1;

            const int n = (int)pt.size();
            if (n < 4) return c;

            std::vector<LV> p4(n);
            for (int i = 0; i < n; ++i)
            {
                const float m = (std::abs(pdgId[i]) == 11) ? kMe : kMmu;
                p4[i] = LV(pt[i], eta[i], phi[i], m);
            }

            // Z1: OSSF pair (same flavour, opposite sign => pdgId[i]==-pdgId[j])
            //     closest to the Z mass
            int a1 = -1, b1 = -1; float best = 1e9f;
            for (int i = 0; i < n; ++i)
              for (int j = i + 1; j < n; ++j)
              {
                  if (pdgId[i] != -pdgId[j]) continue;
                  const float d = std::fabs((p4[i] + p4[j]).M() - kMZ);
                  if (d < best) { best = d; a1 = i; b1 = j; }
              }
            if (a1 < 0) return c;

            // Z2: best remaining OSSF pair by scalar pt sum
            int a2 = -1, b2 = -1; float bestSum = -1.f;
            for (int i = 0; i < n; ++i)
            {
                if (i == a1 || i == b1) continue;
                for (int j = i + 1; j < n; ++j)
                {
                    if (j == a1 || j == b1) continue;
                    if (pdgId[i] != -pdgId[j]) continue;
                    const float s = pt[i] + pt[j];
                    if (s > bestSum) { bestSum = s; a2 = i; b2 = j; }
                }
            }
            if (a2 < 0) return c;

            const LV Z1 = p4[a1] + p4[b1];
            const LV Z2 = p4[a2] + p4[b2];
            const LV H  = Z1 + Z2;

            c.ok = 1;
            c.l1 = a1; c.l2 = b1; c.l3 = a2; c.l4 = b2;
            c.mZ1 = Z1.M(); c.mZ2 = Z2.M();
            c.Z1pt = Z1.Pt(); c.Z2pt = Z2.Pt();
            c.m4l = H.M(); c.pt4l = H.Pt(); c.eta4l = H.Eta(); c.phi4l = H.Phi();

            int nele = 0;
            for (int idx : {a1, b1, a2, b2}) if (std::abs(pdgId[idx]) == 11) ++nele;
            c.channel = (nele == 0) ? 0 : (nele == 4) ? 1 : 2;
            return c;
        },
        {p + "_pt", p + "_eta", p + "_phi", p + "_charge", p + "_pdgId"});

    df = df.Define(out + "_ok",      [](const ZZCand& c){ return c.ok;      }, {carrier});
    df = df.Define(out + "_channel", [](const ZZCand& c){ return c.channel; }, {carrier});
    df = df.Define(out + "_mZ1",     [](const ZZCand& c){ return c.mZ1;     }, {carrier});
    df = df.Define(out + "_mZ2",     [](const ZZCand& c){ return c.mZ2;     }, {carrier});
    df = df.Define(out + "_m4l",     [](const ZZCand& c){ return c.m4l;     }, {carrier});
    df = df.Define(out + "_Z1pt",    [](const ZZCand& c){ return c.Z1pt;    }, {carrier});
    df = df.Define(out + "_Z2pt",    [](const ZZCand& c){ return c.Z2pt;    }, {carrier});
    df = df.Define(out + "_pt",      [](const ZZCand& c){ return c.pt4l;    }, {carrier});
    df = df.Define(out + "_eta",     [](const ZZCand& c){ return c.eta4l;   }, {carrier});
    df = df.Define(out + "_phi",     [](const ZZCand& c){ return c.phi4l;   }, {carrier});
    df = df.Define(out + "_l1",      [](const ZZCand& c){ return c.l1;      }, {carrier});
    df = df.Define(out + "_l2",      [](const ZZCand& c){ return c.l2;      }, {carrier});
    df = df.Define(out + "_l3",      [](const ZZCand& c){ return c.l3;      }, {carrier});
    df = df.Define(out + "_l4",      [](const ZZCand& c){ return c.l4;      }, {carrier});
    return df;
}

// ============================================================================
// Define a trigger-OR flag column from a list of HLT path names (without the
// "HLT_" prefix). Missing paths are skipped so this is safe across eras; if
// none exist the column is defined as false.
// ============================================================================
static RNode DefineTrigOr(RNode df, const std::string& out,
                          const std::vector<std::string>& paths)
{
    const auto cols = df.GetColumnNames();
    auto has = [&](const std::string& c)
    { return std::find(cols.begin(), cols.end(), c) != cols.end(); };

    std::string expr;
    for (const auto& pth : paths)
    {
        const std::string col = "HLT_" + pth;
        if (!has(col)) continue;
        expr += (expr.empty() ? "" : " || ") + col;
    }
    if (expr.empty()) expr = "false";
    return df.Define(out, expr);
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <filelist.txt> <output.root> [nthreads]" << std::endl;
        return 1;
    }

    const std::string input_file_name  = argv[1];
    const std::string output_file_name = argv[2];
    const int nthreads = (argc >= 4) ? std::atoi(argv[3]) : 4;

    if (nthreads > 1) ROOT::EnableImplicitMT(nthreads);
    else              ROOT::DisableImplicitMT();

    // ---- Read the input file list (one path per line; # comments allowed) ----
    std::vector<std::string> inputfiles;
    {
        std::ifstream infile(input_file_name);
        std::string line;
        while (std::getline(infile, line))
        {
            auto first = line.find_first_not_of(" \t");
            if (first == std::string::npos) continue;   // blank
            if (line[first] == '#') continue;           // comment
            // trim trailing whitespace too
            auto last = line.find_last_not_of(" \t\r\n");
            inputfiles.push_back(line.substr(first, last - first + 1));
        }
    }
    if (inputfiles.empty())
    {
        std::cerr << "No input files found in " << input_file_name << std::endl;
        return 1;
    }
    std::cout << "[fourlepton] " << inputfiles.size() << " input file(s), "
              << "nthreads=" << nthreads << std::endl;

    RDataFrame rdf("Events", inputfiles);
    RNode df = rdf;
    ROOT::RDF::Experimental::AddProgressBar(df);

    // ---- Object identification (Run 3 2024) ----
    df = df.Define("Muon_anaID",     muon_run3ID, muon_run3ID_inputs)
           .Define("Electron_anaID", elec_run3ID, elec_run3ID_inputs)
           .Define("Jet_anaID",      jet_run3ID,  jet_run3ID_inputs);

    // ---- Build Good collections ----
    // Loose (tier>=1) leptons for extra-lepton counting / vetoes
    df = RdfUtil::TrimCollection(df, "Muon",     "Muon_anaID>=1",     "GoodLooseMuon",     AnaMuon_props);
    df = RdfUtil::TrimCollection(df, "Electron", "Electron_anaID>=1", "GoodLooseElectron", AnaElectron_props);
    df = RdfUtil::MergeCollections(df, "GoodLooseMuon", "GoodLooseElectron", "GoodLooseLepton");

    // Analysis (tier>=2) leptons -> the 4l candidate leptons
    df = RdfUtil::TrimCollection(df, "Muon",     "Muon_anaID>=2",     "GoodMuon",     AnaMuon_props);
    df = RdfUtil::TrimCollection(df, "Electron", "Electron_anaID>=2", "GoodElectron", AnaElectron_props);
    df = RdfUtil::MergeCollections(df, "GoodMuon", "GoodElectron", "GoodLepton");

    // Jets: build, then overlap-remove against analysis leptons (dR<0.4)
    df = RdfUtil::TrimCollection(df, "Jet", "Jet_anaID", "PreORGoodJet", AnaJet_props);
    df = RdfUtil::TrimCollectionByDeltaR(df, "PreORGoodJet", "GoodLepton", "GoodJet", AnaJet_props, 0.4f);

    // ---- Trigger flags (RunIII 2024 dilepton / trilepton / single-lepton) ----
    df = DefineTrigOr(df, "Trig_DoubleMu",  {"Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8"});
    df = DefineTrigOr(df, "Trig_DoubleEle", {"Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ",
                                             "Ele23_Ele12_CaloIdL_TrackIdL_IsoVL"});
    df = DefineTrigOr(df, "Trig_MuEG",      {"Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ",
                                             "Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ"});
    df = DefineTrigOr(df, "Trig_TriLep",    {"DiMu9_Ele9_CaloIdL_TrackIdL_DZ",
                                             "Mu8_DiEle12_CaloIdL_TrackIdL",
                                             "Ele16_Ele12_Ele8_CaloIdL_TrackIdL",
                                             "TripleMu_12_10_5", "TripleMu_10_5_5_DZ"});
    df = DefineTrigOr(df, "Trig_SingleMu",  {"IsoMu24", "Mu50"});
    df = DefineTrigOr(df, "Trig_SingleEle", {"Ele30_WPTight_Gsf", "Ele32_WPTight_Gsf"});
    df = df.Define("Trig_4l",
        "Trig_DoubleMu || Trig_DoubleEle || Trig_MuEG || Trig_TriLep || "
        "Trig_SingleMu || Trig_SingleEle");

    // ---- Four-lepton preselection ----
    df = df.Filter("nGoodLepton >= 4", "N_{lep}>=4");

    // ---- ZZ -> 4l candidate ----
    df = BuildZZ4l(df, "GoodLepton", "ZZ");

    // ---- Output ----
    auto cols_to_write = RdfUtil::SelectColumnNames(df, {
        "GoodLepton_*", "nGoodLepton",
        "nGoodLooseLepton",
        "GoodJet_*", "nGoodJet",
        "ZZ_*",
        "PuppiMET_pt", "PuppiMET_phi",
        "Trig_*",
        "genWeight", "run", "luminosityBlock", "event",
    },
    // exclude internal helper columns that match the globs above
    {"*_ORmask"});

    // Book the cutflow report BEFORE Snapshot so it is filled in the same
    // event loop (both are lazy; Snapshot triggers the run).
    auto rep = df.Report();

    std::cout << "[fourlepton] writing " << cols_to_write.size()
              << " columns to " << output_file_name << std::endl;
    df.Snapshot("Events", output_file_name, cols_to_write);

    // ---- Cutflow ----
    std::cout << "\n==================== Cutflow ====================" << std::endl;
    rep->Print();

    return 0;
}
