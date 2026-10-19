// jetgen: generate synthetic jet constituents for the jets in a table written
// by dump_jets.py, using synth::JetGenerator (SynthJetGen.h).
//
// Usage: jetgen jets.txt [particles.txt] [seed]

#include "SynthJetGen.h"
#include "Pythia8/Basics.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using Pythia8::Vec4;
using synth::GenResult;

struct JetInput {
  long   iev = 0, event = 0;
  int    ijet = 0;
  double pt = 0, eta = 0, phi = 0, mass = 0, rawFactor = 0;
  int    nConstituents = -1;
  double chHEF = -1, qgl = -1;
  int    partonFlavour = 0, hadronFlavour = 0;
  double pvx = 0, pvy = 0, pvz = 0;

  double ptRaw()   const { return pt * (1. - rawFactor); }
  double massRaw() const { return mass * (1. - rawFactor); }

  synth::JetSpec spec() const {
    synth::JetSpec s;
    s.pt = pt; s.eta = eta; s.phi = phi; s.mass = mass; s.rawFactor = rawFactor;
    s.partonFlavour = partonFlavour; s.qgScore = qgl;
    return s;
  }
};

//--------------------------------------------------------------------------

// Read the whitespace table written by dump_jets.py; columns are taken from
// the "# iev run ..." header so the order can change.
std::vector<JetInput> readJets(const std::string& fname) {
  std::ifstream in(fname);
  if (!in) { std::cerr << "cannot open " << fname << "\n"; exit(1); }
  std::vector<JetInput> jets;
  std::map<std::string, int> col;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    if (line[0] == '#') {
      std::istringstream hs(line.substr(1));
      std::string name;
      std::vector<std::string> names;
      while (hs >> name) names.push_back(name);
      if (!names.empty() && names[0] == "iev")
        for (size_t i = 0; i < names.size(); ++i) col[names[i]] = i;
      continue;
    }
    std::istringstream ls(line);
    std::vector<double> v;
    double x;
    while (ls >> x) v.push_back(x);
    auto get = [&](const char* name, double def) {
      auto it = col.find(name);
      return (it != col.end() && it->second < (int)v.size()) ? v[it->second] : def;
    };
    JetInput j;
    j.iev = get("iev", 0);         j.event = get("event", 0);
    j.ijet = get("ijet", 0);
    j.pt = get("pt", 0);           j.eta = get("eta", 0);
    j.phi = get("phi", 0);         j.mass = get("mass", 0);
    j.rawFactor = get("rawFactor", 0);
    j.nConstituents = get("nConstituents", -1);
    j.chHEF = get("chHEF", -1);    j.qgl = get("qgl", -1);
    j.partonFlavour = get("partonFlavour", 0);
    j.hadronFlavour = get("hadronFlavour", 0);
    j.pvx = get("pvx", 0); j.pvy = get("pvy", 0); j.pvz = get("pvz", 0);
    jets.push_back(j);
  }
  return jets;
}

//--------------------------------------------------------------------------

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " jets.txt [particles.txt] [seed]\n";
    return 1;
  }
  const std::string outName = argc > 2 ? argv[2] : "particles.txt";
  const unsigned int seed = argc > 3 ? std::stoul(argv[3]) : 12345;

  auto jets = readJets(argv[1]);
  std::cerr << "read " << jets.size() << " jets\n";

  synth::JetGenerator gen(seed);
  std::ofstream out(outName);
  out << "# iev event ijet pdgId charge px py pz e\n";

  std::printf("%4s %3s %2s | %7s %6s %6s %6s %4s %5s | %7s %6s %6s %6s %4s %4s %5s | %6s\n",
              "iev", "jet", "qg", "ptRaw", "eta", "phi", "mRaw", "nCon", "chHEF",
              "pt", "eta", "phi", "mass", "nCon", "nCh", "chHF", "ms");
  for (const auto& j : jets) {
    // Same per-jet seed as before the generator moved to SynthJetGen.
    GenResult r = gen.Generate(j.spec(), j.event * 101ul + j.ijet);
    Vec4 sum;
    int nch = 0;
    double ech = 0;
    for (const auto& p : r.particles) {
      out << j.iev << ' ' << j.event << ' ' << j.ijet << ' ' << p.pdgId << ' '
          << p.charge << ' ' << p.px << ' ' << p.py << ' ' << p.pz << ' '
          << p.energy << '\n';
      sum += Vec4(p.px, p.py, p.pz, p.energy);
      if (p.charge != 0) {
        ++nch;
        // Charged hadrons only, to compare with Jet_chHEF.
        int a = std::abs(p.pdgId);
        if (a != 11 && a != 13) ech += p.energy;
      }
    }
    std::printf("%4ld %3d %2s | %7.1f %6.2f %6.2f %6.1f %4d %5.2f | "
                "%7.1f %6.2f %6.2f %6.1f %4zu %4d %5.2f | %6.2f\n",
                j.iev, j.ijet, r.gluon ? "g" : "q",
                j.ptRaw(), j.eta, j.phi, j.massRaw(), j.nConstituents, j.chHEF,
                sum.pT(), sum.eta(), sum.phi(), sum.mCalc(), r.particles.size(),
                nch, sum.e() > 0 ? ech / sum.e() : 0., r.usec / 1000.);
  }
  std::cerr << "wrote " << outName << "\n";
  return 0;
}
