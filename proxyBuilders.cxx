
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
#include <ROOT/REveViewer.hxx>
#include <ROOT/REveViewContext.hxx>
#include <ROOT/REveBox.hxx>
#include <ROOT/REvePointSet.hxx>
#include "CmsNanoClasses.hxx"

extern ROOT::Experimental::REveTrackPropagator* muonPropagator_g;


using namespace ROOT::Experimental;
float EtaToTheta(float eta)
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
      auto track = new REveTrack((TParticle*)(x), 1, muonPropagator_g);
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