#ifndef SynthTracks_h
#define SynthTracks_h

// Synthetic charged-particle tracks for NanoAOD jets, drawn by JetProxyBuilder.
//
// For every jet, synth::JetGenerator (Pythia 8 standalone shower +
// hadronization, see syntGen/SynthJetGen.h) produces a plausible set
// of constituents matched to the jet pT / eta / phi. Charged ones become
// REveTracks with the display's track propagator. They are NOT the
// reconstructed tracks of the event.
//
// The jet seeds the random generator, so a jet always gets the same tracks.

#include "SynthJetGen.h"

#include <ROOT/REveTrack.hxx>
#include <ROOT/REveTrackPropagator.hxx>
#include <ROOT/REveVSDStructs.hxx>

#include "TString.h"

#include <cmath>
#include <type_traits>
#include <utility>
#include <vector>

namespace nanoaod
{
namespace synth_detail
{
// Optional jet accessors: which ones exist depends on the NanoAOD version
// the classes were generated from (e.g. qgl in v9, btagPNetQvG in Run 3).
#define SYNTH_HAS(NAME)                                                                  \
   template <class T, class = void>                                                      \
   struct has_##NAME : std::false_type {};                                               \
   template <class T>                                                                    \
   struct has_##NAME<T, std::void_t<decltype(std::declval<T &>().NAME())>> : std::true_type {};

SYNTH_HAS(rawFactor)
SYNTH_HAS(partonFlavour)
SYNTH_HAS(qgl)
SYNTH_HAS(btagUParTAK4QvG)
SYNTH_HAS(btagPNetQvG)
#undef SYNTH_HAS
} // namespace synth_detail

template <class T>
synth::JetSpec MakeJetSpec(T &j)
{
   using namespace synth_detail;
   synth::JetSpec s;
   s.pt = j.pt();
   s.eta = j.eta();
   s.phi = j.phi();
   s.mass = j.mass();
   if constexpr (has_rawFactor<T>::value)
      s.rawFactor = j.rawFactor();
   if constexpr (has_partonFlavour<T>::value)
      s.partonFlavour = j.partonFlavour();
   // Quark-likeness: prefer the newest tagger present. Scores < 0 mean
   // "not available" for that jet.
   if constexpr (has_btagUParTAK4QvG<T>::value)
      s.qgScore = j.btagUParTAK4QvG();
   else if constexpr (has_btagPNetQvG<T>::value)
      s.qgScore = j.btagPNetQvG();
   else if constexpr (has_qgl<T>::value)
      s.qgScore = j.qgl();
   return s;
}

// Charged synthetic tracks of one jet with pt >= minPt (GeV), starting at the origin.
template <class T>
std::vector<ROOT::Experimental::REveTrack *> MakeSynthTracks(T &jet, ROOT::Experimental::REveTrackPropagator *prop,
                                                            double minPt = 0)
{
   using namespace ROOT::Experimental;

   synth::JetSpec spec = MakeJetSpec(jet);
   synth::GenResult res = synth::JetGenerator::Instance().Generate(spec, synth::SeedFromJet(spec));

   std::vector<REveTrack *> tracks;
   for (const auto &p : res.particles)
   {
      double pt = std::hypot(p.px, p.py);
      if (p.charge == 0 || pt < minPt)
         continue;
      REveRecTrackD rt;
      rt.fV.Set(0., 0., 0.);
      rt.fP.Set(p.px, p.py, p.pz);
      rt.fSign = p.charge > 0 ? 1 : -1;
      auto track = new REveTrack(&rt, prop);
      track->SetTitle(Form("synthetic pdg %d, pt %.2f GeV", p.pdgId, pt));
      track->MakeTrack();
      tracks.push_back(track);
   }
   return tracks;
}

} // namespace nanoaod

#endif
