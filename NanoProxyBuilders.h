#ifndef NanoProxyBuilders_h
#define NanoProxyBuilders_h


#include "ROOT/REveDataCollection.hxx"
#include "ROOT/REveDataSimpleProxyBuilderTemplate.hxx"
#include "ROOT/REveManager.hxx"
#include "ROOT/REveScalableStraightLineSet.hxx"
#include "ROOT/REveViewContext.hxx"
#include <ROOT/REveGeoShape.hxx>
#include <ROOT/REveJetCone.hxx>
#include <ROOT/REvePointSet.hxx>
#include <ROOT/REveProjectionBases.hxx>
#include <ROOT/REveProjectionManager.hxx>
#include <ROOT/REveScene.hxx>
#include <ROOT/REveTableProxyBuilder.hxx>
#include <ROOT/REveTableInfo.hxx>
#include <ROOT/REveTrack.hxx>
#include <ROOT/REveTrackPropagator.hxx>
#include <ROOT/REveTrackProjected.hxx>
#include <ROOT/REveViewer.hxx>
#include <ROOT/REveViewContext.hxx>
#include <ROOT/REveBox.hxx>
#include <ROOT/REvePointSet.hxx>
#include "CmsNanoClasses.hxx"
#include "FWDataCollection.h"
#ifdef CMSNANO_SYNTH  // make SYNTH=1, the default
#include "SynthTracks.h"
#endif

#include "TParticle.h"

// Proxy builders for the generated NanoAOD classes. They are templates on the
// item class, instantiated in EvdGuiLinkDef.h from the proxyBuilder entries in
// the config, e.g. "nanoaod::JetProxyBuilder<nanoaod::Jet>". They live in
// namespace nanoaod so they don't clash with VSDNano's builders of the same name.

namespace nanoaod
{

using namespace ROOT::Experimental;

inline float EtaToTheta(float eta)
{
   using namespace TMath;

   if (eta < 0)
      return Pi() - 2*ATan(Exp(- Abs(eta)));
   else
      return 2*ATan(Exp(- Abs(eta)));
}

///////////////////////////////////////////////////////////////////////////////////////
template<typename T>
class ElectronProxyBuilder : public REveDataSimpleProxyBuilderTemplate<T>
{
   using REveDataSimpleProxyBuilderTemplate<T>::BuildItem;
   void BuildItem(const T& c_electron, int /*idx*/, REveElement* iItemHolder, const REveViewContext* context) override
   {
      T& electron = (T&)(c_electron);
      int pdg = 11 * electron.charge();

      float theta = EtaToTheta(electron.eta());
      float phi = electron.phi();
      float p = electron.pt() / TMath::Sin(theta);
      float px = p * TMath::Cos(theta) * TMath::Cos(phi);
      float py = p * TMath::Cos(theta) * TMath::Sin(phi);
      float pz = p * TMath::Sin(theta);
      float etot = p; // ???
      auto x = new TParticle(pdg, 0, 0, 0, 0, 0,
                             px, py, pz, p,
                             0, 0, 0, 0);

      // printf("==============  BUILD track %s (pt=%f, eta=%f) \n", iItemHolder->GetCName(), p.Pt(), p.Eta());
      auto track = new REveTrack((TParticle *)(x), 1, context->GetPropagator());
      track->MakeTrack();
      this->SetupAddElement(track, iItemHolder, true);
      // iItemHolder->AddElement(track);
      // track->SetName(Form("element %s id=%d", iItemHolder->GetCName(), track->GetElementId()));
   }
};

/////////////////////////////////////////////////////////////////////////////////////
template<typename T>
class VertexProxyBuilder : public REveDataSimpleProxyBuilderTemplate<T>
{
   using REveDataSimpleProxyBuilderTemplate<T>::BuildItem;
   void BuildItem(const T& c_pv, int /*idx*/, REveElement* iItemHolder, const REveViewContext* context) override
   {

      T& pv = (T&)(c_pv);
      REvePointSet* ps = new REvePointSet();
      ps->SetNextPoint(pv.x(), pv.y(), pv.z());
      ps->SetMarkerSize(4);
      this->SetupAddElement(ps, iItemHolder);
   }
};

/////////////////////////////////////////////////////////////////////////////////////
template<typename T>
class MuonProxyBuilder : public REveDataSimpleProxyBuilderTemplate<T>
{
   REveTrackPropagator *m_muonPropagator = nullptr;

   REveTrackPropagator *muonPropagator()
   {
      if (!m_muonPropagator) {
         m_muonPropagator = new REveTrackPropagator();
         m_muonPropagator->SetMagFieldObj(new REveMagFieldDuo(350, -3.5, 2.0));
         m_muonPropagator->SetMaxR(850);
         m_muonPropagator->SetMaxZ(1100);
         m_muonPropagator->SetMaxOrbs(6);
         m_muonPropagator->IncRefCount();
      }
      return m_muonPropagator;
   }

   using REveDataSimpleProxyBuilderTemplate<T>::BuildItem;
   void BuildItem(const T& c_electron, int /*idx*/, REveElement* iItemHolder, const REveViewContext* context) override
   {

      T& electron = (T&)(c_electron);
      int pdg = 11 * electron.charge();

      float theta = EtaToTheta(electron.eta());
      float phi = electron.phi();
      float p = electron.pt()/TMath::Sin(theta);
      float px = p * TMath::Cos(theta) * TMath::Cos(phi);
      float py = p * TMath::Cos(theta) * TMath::Sin(phi);
      float pz = p * TMath::Sin(theta);
      float etot = p; // ???
         auto x = new TParticle(pdg, 0, 0, 0, 0, 0,
                                       px, py, pz,  p,
                                       0, 0, 0, 0 );

      // printf("==============  BUILD track %s (pt=%f, eta=%f) \n", iItemHolder->GetCName(), p.Pt(), p.Eta());
      auto track = new REveTrack((TParticle*)(x), 1, muonPropagator());
      track->MakeTrack();
      this->SetupAddElement(track, iItemHolder, true);
   }
};
/////////////////////////////////////////////////////////////////////////////////////
template<typename T>
class JetProxyBuilder: public REveDataSimpleProxyBuilderTemplate<T>
{
   struct Cell {
      float thetaMin;
      float thetaMax;
      float phiMin;
      float phiMax;
   };

   bool HaveSingleProduct() const override { return false; }

   // Depth in the 2D projections (evd_main.cc: geometry -4, event objects 0).
   // Jet cones and energy boxes are drawn behind the synthetic tracks, which
   // stay behind the reconstructed muons and electrons.
   static constexpr int   kJetLayer = -2;

   using REveDataProxyBuilderBase::SetCollection;
   void SetCollection(REveDataCollection *collection) override
   {
      REveDataProxyBuilderBase::SetCollection(collection);
      // The projection manager uses the layer as the current depth when
      // projecting this collection's items.
      collection->SetLayer(kJetLayer);
#ifdef CMSNANO_SYNTH
      if (auto fwc = dynamic_cast<FWDataCollection *>(collection))
      {
         fwc->assertParameter(kSynthTracks, true);
         fwc->assertParameter(kSynthTrackColor, long(kGray + 2));
         fwc->assertParameter(kSynthTrackMinPt, 1., 0., 1000.);
      }
#endif
   }

#ifdef CMSNANO_SYNTH
   // GUI parameter of the jet collection: draw synthetic charged tracks
   // (Pythia 8 constituents, see SynthTracks.h) inside each jet.
   static constexpr const char *kSynthTracks = "SynthTracks";
   // ROOT color index of the synthetic tracks, e.g. 922 gray, 1 black, 616 magenta.
   // Not green: that is the selection highlight.
   static constexpr const char *kSynthTrackColor = "SynthTrackColor";
   // Synthetic tracks with lower pt (GeV) are not drawn.
   static constexpr const char *kSynthTrackMinPt = "SynthTrackMinPt";

   static constexpr float kSynthTrackDepth = -1;

   // The whole jet item is projected at kJetLayer; move the projected
   // synthetic tracks in front of it.
   using REveDataProxyBuilderBase::Build;
   void Build() override
   {
      REveDataProxyBuilderBase::Build();
      for (auto *pp : this->m_products)
         if (auto pable = dynamic_cast<REveProjectable *>(pp->m_elements))
            for (auto *proj : pable->RefProjecteds())
               raiseSynthTracks(proj->GetProjectedAsElement());
   }

   static void raiseSynthTracks(REveElement *el)
   {
      for (auto *c : el->RefChildren())
      {
         if (auto tp = dynamic_cast<REveTrackProjected *>(c))
         {
            tp->SetDepth(kSynthTrackDepth);
            tp->UpdateProjection();
         }
         else
         {
            raiseSynthTracks(c);
         }
      }
   }

   bool drawSynthTracks()
   {
      auto fwc = dynamic_cast<FWDataCollection *>(this->Collection());
      return fwc && fwc->getBoolParameter(kSynthTracks);
   }
#endif // CMSNANO_SYNTH

   void makeBarrelCell( Cell &cellData, float &offset , float towerH, float *pnts)
   {
      using namespace TMath;

      float r1 = offset;
      float r2 = r1 + towerH*Sin(cellData.thetaMin);
      float z1In, z1Out, z2In, z2Out;

      z1In  = r1/Tan(cellData.thetaMax);
      z1Out = r2/Tan(cellData.thetaMax);
      z2In  = r1/Tan(cellData.thetaMin);
      z2Out = r2/Tan(cellData.thetaMin);

      float cos1 = Cos(cellData.phiMin);
      float sin1 = Sin(cellData.phiMin);
      float cos2 = Cos(cellData.phiMax);
      float sin2 = Sin(cellData.phiMax);

      // 0
      pnts[0] = r1*cos2;
      pnts[1] = r1*sin2;
      pnts[2] = z1In;
      pnts += 3;
      // 1
      pnts[0] = r1*cos1;
      pnts[1] = r1*sin1;
      pnts[2] = z1In;
      pnts += 3;
      // 2
      pnts[0] = r1*cos1;
      pnts[1] = r1*sin1;
      pnts[2] = z2In;
      pnts += 3;
      // 3
      pnts[0] = r1*cos2;
      pnts[1] = r1*sin2;
      pnts[2] = z2In;
      pnts += 3;
      //---------------------------------------------------
      // 4
      pnts[0] = r2*cos2;
      pnts[1] = r2*sin2;
      pnts[2] = z1Out;
      pnts += 3;
      // 5
      pnts[0] = r2*cos1;
      pnts[1] = r2*sin1;
      pnts[2] = z1Out;
      pnts += 3;
      // 6
      pnts[0] = r2*cos1;
      pnts[1] = r2*sin1;
      pnts[2] = z2Out;
      pnts += 3;
      // 7
      pnts[0] = r2*cos2;
      pnts[1] = r2*sin2;
      pnts[2] = z2Out;


      offset += towerH*Sin(cellData.thetaMin);

   }// end RenderBarrelCell

   void makeEndCapCell(Cell &cellData, float &offset, float towerH , float *pnts)
   {
      using namespace TMath;
      float z1, r1In, r1Out, z2, r2In, r2Out;

      z1    = offset;
      z2    = z1 + towerH;

      r1In  = z1*Tan(cellData.thetaMin);
      r2In  = z2*Tan(cellData.thetaMin);
      r1Out = z1*Tan(cellData.thetaMax);
      r2Out = z2*Tan(cellData.thetaMax);

      float cos2 = Cos(cellData.phiMin);
      float sin2 = Sin(cellData.phiMin);
      float cos1 = Cos(cellData.phiMax);
      float sin1 = Sin(cellData.phiMax);

      // 0
      pnts[0] = r1In*cos1;
      pnts[1] = r1In*sin1;
      pnts[2] = z1;
      pnts += 3;
      // 1
      pnts[0] = r1In*cos2;
      pnts[1] = r1In*sin2;
      pnts[2] = z1;
      pnts += 3;
      // 2
      pnts[0] = r2In*cos2;
      pnts[1] = r2In*sin2;
      pnts[2] = z2;
      pnts += 3;
      // 3
      pnts[0] = r2In*cos1;
      pnts[1] = r2In*sin1;
      pnts[2] = z2;
      pnts += 3;
      //---------------------------------------------------
      // 4
      pnts[0] = r1Out*cos1;
      pnts[1] = r1Out*sin1;
      pnts[2] = z1;
      pnts += 3;
      // 5
      pnts[0] = r1Out*cos2;
      pnts[1] = r1Out*sin2;
      pnts[2] = z1;
      pnts += 3;
      // 6
      pnts[0] = r2Out*cos2;
      pnts[1] = r2Out*sin2;
      pnts[2] = z2;
      pnts += 3;
      // 7
      pnts[0] = r2Out*cos1;
      pnts[1] = r2Out*sin1;
      pnts[2] = z2;


      if (z1 > 0)
         offset += towerH * Cos(cellData.thetaMin);
      else
         offset -= towerH * Cos(cellData.thetaMin);

   } // end makeEndCapCell


   using REveDataSimpleProxyBuilderTemplate<T>::BuildItemViewType;
   void BuildItemViewType(const T& cdj, int idx, REveElement* iItemHolder,
                      const std::string& viewType, const REveViewContext* context) override
   {
      T& dj = (T&)(cdj);
      auto jet = new REveJetCone();
      jet->SetCylinder(context->GetMaxR() -5, context->GetMaxZ());
      jet->AddEllipticCone(dj.eta(), dj.phi(), 0.2, 0.2);
      this->SetupAddElement(jet, iItemHolder, true);
      jet->SetTitle(Form("jet %d", idx));
      printf("make jet %d\n", idx);
      
      
      static const float_t offr = 5;
      float r_ecal = context->GetMaxR() + offr;
      float z_ecal = context->GetMaxZ() + offr;
      float energyScale =  5.f;
      float  transAngle = abs(atan(r_ecal/z_ecal));
      double theta = EtaToTheta(dj.eta());
      double phi = dj.phi();

      Cell cell;
      // hardcoded cell size
      float ad = 0.02;
      // thetaMin => etaMax, thetaMax => thetaMin
      cell.thetaMax = EtaToTheta(dj.eta() - ad);
      cell.thetaMin = EtaToTheta(dj.eta() + ad);
      cell.phiMin = phi - ad;
      cell.phiMax = phi + ad;
      float pnts[24];

      // an example of slices
      std::vector<float> sliceVals;
      sliceVals.push_back( dj.pt() * (1 - dj.chHEF()));
      sliceVals.push_back( dj.pt() * dj.chHEF());

      if (theta < transAngle || (3.14-theta) < transAngle)
      {
          printf("Set points for ENDCAP  REveBox [%d] ======================= \n", idx);
         float offset = TMath::Sign(context->GetMaxZ(), dj.eta());
         for (auto &val : sliceVals) {
            offset +=  TMath::Sign(offr, dj.eta());
            makeEndCapCell(cell, offset,  TMath::Sign(val*energyScale, dj.eta()), &pnts[0]);
            REveBox* reveBox = new REveBox();
            reveBox->SetVertices(pnts);
            this->SetupAddElement(reveBox, iItemHolder, true);
         }
      }
      else
      {
         printf("Set points for BARREL  REveBox [%d] ======================= \n", idx);
         float offset = context->GetMaxR();
         for (auto &val : sliceVals) {
            offset += offr;
            makeBarrelCell(cell, offset, val*energyScale, &pnts[0]);
            REveBox* reveBox = new REveBox();
            reveBox->SetVertices(pnts);
            this->SetupAddElement(reveBox, iItemHolder, true);
            reveBox->SetTitle(Form("jet %d", idx)); // amt this is workaround and should be unnecessary
         }
      }

#ifdef CMSNANO_SYNTH
      if (drawSynthTracks())
      {
         auto fwc = dynamic_cast<FWDataCollection *>(this->Collection());
         Color_t color = Color_t(fwc->getLongParameter(kSynthTrackColor));
         double minPt = fwc->getDoubleParameter(kSynthTrackMinPt);
         for (auto track : MakeSynthTracks(dj, context->GetPropagator(), minPt))
         {
            track->SetMainColor(color);
            this->SetupAddElement(track, iItemHolder, false);
         }
      }
#endif
   }


   using REveDataProxyBuilderBase::LocalModelChanges;

   void LocalModelChanges(int idx, REveElement* el, const REveViewContext* ctx) override
   {
      printf("LocalModelChanges jet %s ( %s )\n", el->GetCName(), el->FirstChild()->GetCName());
      REveJetCone* cone = dynamic_cast<REveJetCone*>(el->FirstChild());
      cone->SetLineColor(cone->GetMainColor());
   }
};


//==============================================================================
template<typename T>
class METProxyBuilder : public REveDataSimpleProxyBuilderTemplate<T>
{
public:
   virtual bool HaveSingleProduct() const override { return false; }

   using REveDataSimpleProxyBuilderTemplate<T>::BuildItemViewType;
   virtual void BuildItemViewType(const T &c_met, int /*idx*/, ROOT::Experimental::REveElement *iItemHolder,
                                  const std::string &viewType, const REveViewContext *context) override
   {
      T& met = (T&)(c_met);
      using namespace TMath;
      double phi = met.phi();
     // double theta = 1;//EtaToTheta(met.eta());
      double theta = TMath::Pi()/2;
      double size = 1.f;

      REveScalableStraightLineSet *marker = new REveScalableStraightLineSet("MET marker");
      marker->SetLineWidth(2);
      marker->SetAlwaysSecSelect(false);

      this->SetupAddElement(marker, iItemHolder);

      float offr = 5;
      float r_ecal = context->GetMaxR() + offr;
      float z_ecal = context->GetMaxZ() + offr;
      float energyScale = 5.f;

      if (viewType.compare(0, 3, "Rho") == 0)
      {
         // body
         double r0 = r_ecal;
         //if (TMath::Abs(met.eta()) < abs(atan(r_ecal / z_ecal)))
         /*
         if (true)
         {
            r0 = r_ecal / sin(theta);
         }
         else
         {
            r0 = z_ecal / fabs(cos(theta));
         }*/
         marker->SetScaleCenter(0., Sign(r0 * sin(theta), phi), r0 * cos(theta));
         double r1 = r0 + 1;
         marker->AddLine(0., Sign(r0 * sin(theta), phi), r0 * cos(theta),
                         0., Sign(r1 * sin(theta), phi), r1 * cos(theta));
         // arrow pointer
         double r2 = r1 - 0.1;
         double dy = 0.05 * size;
         marker->AddLine(0., Sign(r2 * sin(theta) + dy * cos(theta), phi), r2 * cos(theta) - dy * sin(theta),
                         0., Sign(r1 * sin(theta), phi), r1 * cos(theta));
         dy = -dy;
         marker->AddLine(0., Sign(r2 * sin(theta) + dy * cos(theta), phi), r2 * cos(theta) - dy * sin(theta),
                         0., Sign(r1 * sin(theta), phi), r1 * cos(theta));

         // segment
         /*
         addRhoZEnergyProjection(this, iItemHolder, r_ecal - 1, z_ecal - 1,
                                 theta - 0.04, theta + 0.04,
                                 phi);*/
      }
      else
      {
         // body
         double r0 = r_ecal;
         double r1 = r0 + 1;
         marker->SetScaleCenter(r0 * cos(phi), r0 * sin(phi), 0);
         marker->AddLine(r0 * cos(phi), r0 * sin(phi), 0,
                         r1 * cos(phi), r1 * sin(phi), 0);

         // arrow pointer, xy  rotate offset point ..
         double r2 = r1 - 0.1;
         double dy = 0.05 * size;

         marker->AddLine(r2 * cos(phi) - dy * sin(phi), r2 * sin(phi) + dy * cos(phi), 0,
                         r1 * cos(phi), r1 * sin(phi), 0);
         dy = -dy;
         marker->AddLine(r2 * cos(phi) - dy * sin(phi), r2 * sin(phi) + dy * cos(phi), 0,
                         r1 * cos(phi), r1 * sin(phi), 0);

         // segment
         double min_phi = phi - M_PI / 36 / 2;
         double max_phi = phi + M_PI / 36 / 2;
         REveGeoManagerHolder gmgr(REveGeoShape::GetGeoManager());
         //REveGeoShape *element = getShape("spread", new TGeoTubeSeg(r0 - 2, r0, 1, min_phi * 180 / M_PI, max_phi * 180 / M_PI), 0);
         //element->SetPickable(kTRUE);
         //element->SetMainTransparency(90);
         //xsSetupAddElement(element, iItemHolder);
      }
      // float value = met.et();
      float value = met.pt();
      marker->SetScale(energyScale * value);
   }
};

} // namespace nanoaod

#endif
