#ifndef SynthJetGen_h
#define SynthJetGen_h

// Synthetic jet constituents from Pythia 8 standalone shower + hadronization.
//
// The header has no Pythia / FastJet includes, so it can be parsed by
// rootcling and included from dictionary headers. The implementation lives in
// libSynthJetGen.

#include <memory>
#include <vector>

namespace synth
{

struct JetSpec
{
   double pt = 0, eta = 0, phi = 0, mass = 0;
   double rawFactor = 0;      // NanoAOD Jet_rawFactor; 0 if unknown
   int    partonFlavour = 0;  // MC truth; 0 if unknown (data)
   double qgScore = -1;       // quark-like in [0,1] (qgl, QvG); -1 if unknown

   double ptRaw()   const { return pt * (1. - rawFactor); }
   double massRaw() const { return mass * (1. - rawFactor); }
};

struct Particle
{
   int    pdgId;
   double charge;
   double px, py, pz, energy;
};

struct GenResult
{
   std::vector<Particle> particles;
   bool   gluon = false;
   int    nAllVisible = 0;  // visible final state of the whole parton pair
   double usec = 0;
};

class JetGenerator
{
public:
   explicit JetGenerator(unsigned int seed = 12345);
   ~JetGenerator();

   // Reproducible for a given (generator seed, jetSeed).
   GenResult Generate(const JetSpec &jet, unsigned long jetSeed);

   // Shared instance, initialized on first use (~50 ms for Pythia init).
   static JetGenerator &Instance();

private:
   struct Impl;
   std::unique_ptr<Impl> m_impl;
};

// Seed from jet kinematics, so the same jet always gets the same particles.
unsigned long SeedFromJet(const JetSpec &jet);

} // namespace synth

#endif
