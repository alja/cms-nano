// For each jet:
//   1. Put a colour-connected q qbar (or g g) pair back-to-back along +-x,
//      i.e. the seed parton is central (eta = 0, phi = 0), E = raw jet pT.
//   2. forceTimeShower + hadronize + decay.
//   3. Cluster visible final-state particles with anti-kT R = 0.4 and take
//      the jet closest to the seed parton.
//   4. Scale constituent 3-momenta to the raw target pT, rotate to the
//      target phi and boost along z to the target rapidity. Rotation and
//      longitudinal boost keep pT and the (y, phi) shape of the jet.

#include "SynthJetGen.h"

#include "Pythia8/Pythia.h"
#include "fastjet/ClusterSequence.hh"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#ifndef PYTHIA8_XMLDOC
#define PYTHIA8_XMLDOC "../share/Pythia8/xmldoc"
#endif

using Pythia8::Event;
using Pythia8::Pythia;
using Pythia8::sqrtpos;
using Pythia8::Vec4;

namespace synth
{

struct JetGenerator::Impl
{
   Pythia        pythia;
   unsigned long baseSeed;

   explicit Impl(unsigned int seed) : pythia(PYTHIA8_XMLDOC, false), baseSeed(seed)
   {
      fastjet::ClusterSequence::set_fastjet_banner_stream(nullptr);
      pythia.readString("ProcessLevel:all = off");
      pythia.readString("Check:event = off");
      // Showers are done by forceTimeShower; don't redo them.
      pythia.readString("PartonLevel:FSRinResonances = off");
      // Standard LHC convention: K0S, Lambda, ... (c*tau > 10 mm) are stable.
      pythia.readString("ParticleDecays:limitTau0 = on");
      pythia.readString("ParticleDecays:tau0Max = 10");
      pythia.readString("Next:numberShowInfo = 0");
      pythia.readString("Next:numberShowProcess = 0");
      pythia.readString("Next:numberShowEvent = 0");
      pythia.readString("Print:quiet = on");
      pythia.readString("Random:setSeed = on");
      pythia.readString("Random:seed = " + std::to_string(seed % 900000000));
      if (!pythia.init())
      {
         std::cerr << "synth::JetGenerator: Pythia init failed\n";
         std::exit(1);
      }
   }

   void fillPartons(bool gluon, double ee)
   {
      Event &event = pythia.event;
      event.reset();
      if (gluon)
      {
         event.append(21, 23, 101, 102, ee, 0., 0., ee, 0.);
         event.append(21, 23, 102, 101, -ee, 0., 0., ee, 0.);
      }
      else
      {
         const int id = 2;
         const double mm = pythia.particleData.m0(id);
         const double pp = sqrtpos(ee * ee - mm * mm);
         event.append(id, 23, 101, 0, pp, 0., 0., ee, mm);
         event.append(-id, 23, 0, 101, -pp, 0., 0., ee, mm);
      }
   }
};

JetGenerator::JetGenerator(unsigned int seed) : m_impl(new Impl(seed)) {}

JetGenerator::~JetGenerator() = default;

JetGenerator &JetGenerator::Instance()
{
   static JetGenerator gen;
   return gen;
}

GenResult JetGenerator::Generate(const JetSpec &jet, unsigned long jetSeed)
{
   auto t0 = std::chrono::steady_clock::now();
   GenResult res;
   Pythia &pythia = m_impl->pythia;
   pythia.rndm.init(int((m_impl->baseSeed * 1000003ul + jetSeed) % 900000000ul) + 1);

   // Quark vs gluon: MC truth if present, else the quark-likeness score.
   if (jet.partonFlavour != 0)
      res.gluon = (jet.partonFlavour == 21);
   else
      res.gluon = (jet.qgScore >= 0 && jet.qgScore < 0.5);

   const double ptTarget = std::max(jet.ptRaw(), 5.);
   const double ee = ptTarget;

   for (int attempt = 0; attempt < 10; ++attempt)
   {
      m_impl->fillPartons(res.gluon, ee);
      pythia.event[1].scale(ee);
      pythia.event[2].scale(ee);
      pythia.forceTimeShower(1, 2, ee);
      if (!pythia.next())
         continue;

      std::vector<fastjet::PseudoJet> input;
      const Event &ev = pythia.event;
      for (int i = 0; i < ev.size(); ++i)
      {
         if (!ev[i].isFinal() || !ev[i].isVisible())
            continue;
         fastjet::PseudoJet p(ev[i].px(), ev[i].py(), ev[i].pz(), ev[i].e());
         p.set_user_index(i);
         input.push_back(p);
      }
      res.nAllVisible = input.size();

      fastjet::ClusterSequence cs(input, fastjet::JetDefinition(fastjet::antikt_algorithm, 0.4));
      auto jets = cs.inclusive_jets(1.0);
      // Seed parton direction: eta = 0, phi = 0.
      const fastjet::PseudoJet seedDir(1., 0., 0., 1.);
      int best = -1;
      double bestDR = 0.4;
      for (size_t k = 0; k < jets.size(); ++k)
      {
         double dr = jets[k].delta_R(seedDir);
         if (dr < bestDR)
         {
            bestDR = dr;
            best = k;
         }
      }
      if (best < 0)
         continue;

      const fastjet::PseudoJet &gj = jets[best];
      const auto constituents = gj.constituents();
      const double scale = ptTarget / gj.pt();
      // Rapidity the scaled jet must be boosted to.
      const double mT = std::sqrt(ptTarget * ptTarget + jet.massRaw() * jet.massRaw());
      const double yTarget = std::asinh(ptTarget / mT * std::sinh(jet.eta));

      auto scaled = [&](const Pythia8::Particle &p) {
         Vec4 v(p.px() * scale, p.py() * scale, p.pz() * scale, 0.);
         v.e(std::sqrt(v.pAbs2() + p.m() * p.m()));
         return v;
      };
      Vec4 pjScaled;
      for (auto &c : constituents)
         pjScaled += scaled(ev[c.user_index()]);
      const double dy = yTarget - pjScaled.rap();
      const double dphi = jet.phi - pjScaled.phi();

      for (auto &c : constituents)
      {
         const Pythia8::Particle &p = ev[c.user_index()];
         Vec4 v = scaled(p);
         v.rot(0., dphi);
         v.bst(0., 0., std::tanh(dy));
         res.particles.push_back({p.id(), p.charge(), v.px(), v.py(), v.pz(), v.e()});
      }
      break;
   }
   res.usec = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
   return res;
}

unsigned long SeedFromJet(const JetSpec &jet)
{
   // FNV-1a over the float bit patterns of pt, eta, phi.
   unsigned long h = 1469598103934665603ul;
   for (float f : {float(jet.pt), float(jet.eta), float(jet.phi)})
   {
      unsigned int bits;
      std::memcpy(&bits, &f, sizeof bits);
      h = (h ^ bits) * 1099511628211ul;
   }
   return h;
}

} // namespace synth
