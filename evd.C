#include "ROOT/REveDataClasses.hxx"
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

#include "TTree.h"
#include "TGeoTube.h"
#include "TList.h"
#include "TParticle.h"
#include "TRandom.h"
#include "TApplication.h"
#include "TMathBase.h"


// globals
ROOT::Experimental::REveManager* eveMng;
ROOT::Experimental::REveProjectionManager* mngRhoZ;
ROOT::Experimental::REveViewContext* viewContext;
ROOT::Experimental::REveTrackPropagator* muonPropagator_g;


using namespace ROOT::Experimental;


Float_t EtaToTheta(Float_t eta)
{
   using namespace TMath;

   if (eta < 0)
      return Pi() - 2*ATan(Exp(- Abs(eta)));
   else
      return 2*ATan(Exp(- Abs(eta)));
}

class ElectronProxyBuilder : public REveDataSimpleProxyBuilderTemplate<nanoaod::Electron>
{
   using REveDataSimpleProxyBuilderTemplate<nanoaod::Electron>::Build;
   void Build(const nanoaod::Electron& c_electron, int /*idx*/, REveElement* iItemHolder, const REveViewContext* context) override
   {

      nanoaod::Electron& electron = (nanoaod::Electron&)(c_electron);
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
      auto track = new REveTrack((TParticle*)(x), 1, context->GetPropagator());
      track->MakeTrack();
      SetupAddElement(track, iItemHolder, true);
      // iItemHolder->AddElement(track);
      //track->SetName(Form("element %s id=%d", iItemHolder->GetCName(), track->GetElementId()));
   }
};

class MuonProxyBuilder : public REveDataSimpleProxyBuilderTemplate<nanoaod::Muon>
{
   using REveDataSimpleProxyBuilderTemplate<nanoaod::Muon>::Build;
   void Build(const nanoaod::Muon& c_electron, int /*idx*/, REveElement* iItemHolder, const REveViewContext* context) override
   {

      nanoaod::Muon& electron = (nanoaod::Muon&)(c_electron);
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
      SetupAddElement(track, iItemHolder, true);
      // iItemHolder->AddElement(track);
      // track->SetName(Form("element %s id=%d", iItemHolder->GetCName(), track->GetElementId()));
   }
};

class JetProxyBuilder: public REveDataSimpleProxyBuilderTemplate<nanoaod::Jet>
{
   bool HaveSingleProduct() const override { return false; }

   using REveDataSimpleProxyBuilderTemplate<nanoaod::Jet>::BuildViewType;

   void BuildViewType(const nanoaod::Jet& cdj, int idx, REveElement* iItemHolder,
                      std::string viewType, const REveViewContext* context) override
   {

      //AMT tmp hack ...
      nanoaod::Jet& dj = (nanoaod::Jet&)(cdj);

      auto jet = new REveJetCone();
      jet->SetCylinder(context->GetMaxR(), context->GetMaxZ());
      jet->AddEllipticCone(dj.eta(), dj.phi(), 0.2, 0.2);
      SetupAddElement(jet, iItemHolder, true);
      jet->SetLineColor(jet->GetMainColor());

      float  size  = 50.f * dj.pt(); // values are saved in scale
      double theta = EtaToTheta(dj.eta());
      //      printf("BuildViewType >>> %s jet theta =  %f, phi = %f \n",  iItemHolder->GetCName(), theta, dj.phi());
      double phi = dj.phi();


      if (viewType == "RhoZProjected" )
      {
         static const float_t offr = 6;
         float r_ecal = context->GetMaxR() + offr;
         float z_ecal = context->GetMaxZ() + offr;

         float  transAngle = abs(atan(r_ecal/z_ecal));
         double r = 0;
         bool debug = false;
         if (theta < transAngle || 3.14-theta < transAngle)
         {
            z_ecal = context->GetMaxZ() + offr/transAngle;
            r = z_ecal/fabs(cos(theta));
         }
         else
         {
            debug = true;
            r = r_ecal/sin(theta);
         }
         REveVector p1(0, TMath::Sign( r*fabs(sin(theta)), phi),  r*cos(theta));
         r+=size;
         REveVector p2(0, TMath::Sign(r*fabs(sin(theta)), phi),  r*cos(theta));

         auto marker = new REveScalableStraightLineSet("jetline");
         marker->SetScaleCenter(p1.fX, p1.fY, p1.fZ);
         marker->AddLine(p1, p2);
         marker->SetLineWidth(4);
         if (debug)
             marker->AddMarker(0, 0.9);

         SetupAddElement(marker, iItemHolder, true);
         marker->SetName(Form("line %s %d", Collection()->GetCName(), idx));
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

//==============================================================================

//==============================================================================


class CollectionManager
{
private:
   nanoaod::Event             *m_event{nullptr};
   REveElement               *m_collections{nullptr};

   std::vector<REveDataProxyBuilderBase *> m_builders;

public:
   CollectionManager(nanoaod::Event* event) : m_event(event)
   {
       m_collections = eveMng->GetScenes()->FindChild("Collections");
       //      m_collections = eveMng->SpawnNewScene("Collections", "Collections");
   }


   void LoadCurrentEventInCollection(REveDataCollection* rdc)
   {
      rdc->ClearItems();
      rdc->DestroyElements();
      nanoaod::MamaCollection& mc = m_event->RefColl(rdc->GetCName());
      std::string cname = rdc->GetName();
      printf("-------- LoadCurrentEventInCollection size %d\n",mc.get_n_entries() );
      for (int i = 0; i < mc.get_n_entries(); ++i)
      {
         TString pname(Form("%s %2d",  cname.c_str(), i));
         printf("access item %d \n", i);
         rdc->AddItem(mc.get_item(i), pname.Data(), "");
      }
   }

   void RenewEvent()
   {
      for (auto &el: m_collections->RefChildren())
      {
         auto c = dynamic_cast<REveDataCollection *>(el);
         LoadCurrentEventInCollection(c);
      }

      for (auto proxy : m_builders)
      {
         proxy->Build();
      }

   }
   REveDataProxyBuilderBase* makeGLBuilderForType(TClass* c)
   {
      // AMT can we do this with gRoot->ProcessLine() instead
      std::string cname = c->GetName();
      if (cname == "nanoaod::Jet")
      {
          return new JetProxyBuilder();
      }
      else if (cname == "nanoaod::Electron")
      {
          return new ElectronProxyBuilder();
      }
      else if (cname == "nanoaod::Muon")
      {
          return new MuonProxyBuilder();
      }
      else
      {
         return nullptr;
      }
   }

   void  addCollection(std::string name, Color_t ccol = kBlue, bool showInTable=false) {
      nanoaod::MamaCollection& mc = m_event->RefColl(name);
      auto rdc = new REveDataCollection (mc.get_class_name());
      rdc->SetItemClass(mc.get_item_class());
      rdc->SetMainColor(ccol);
      m_collections->AddElement(rdc);
      LoadCurrentEventInCollection(rdc);
      printf("add collection instant proxy builders\n");

      if (1) {
         // GL view types
         auto glBuilder = makeGLBuilderForType(rdc->GetItemClass());
         if (glBuilder)
         {
            glBuilder->SetCollection(rdc);
            glBuilder->SetHaveAWindow(true);
            for (auto &scene: eveMng->GetScenes()->RefChildren())
            {
               REveElement *product = glBuilder->CreateProduct(scene->GetTitle(), viewContext);

               if (strncmp(scene->GetCTitle(), "Table", 5) == 0) continue;

               if (!strncmp(scene->GetCTitle(), "RhoZProjected", 8))
               {
                  mngRhoZ->ImportElements(product, scene);
                  continue;
               }
               else if ((!strncmp(scene->GetCName(), "Event scene", 8)))
               {
                  scene->AddElement(product);
               }
            }
            m_builders.push_back(glBuilder);
            glBuilder->Build();
         }
      }
      if (1){
         // Table view types
         auto tableBuilder = new REveTableProxyBuilder();
         tableBuilder->SetHaveAWindow(true);
         tableBuilder->SetCollection(rdc);
         REveElement* tablep = tableBuilder->CreateProduct("table-type", viewContext);
         auto tableMng =  viewContext->GetTableViewInfo();
         if (showInTable)
         {
            tableMng->SetDisplayedCollection(rdc->GetElementId());
         }
         tableMng->AddDelegate([=]() { tableBuilder->ConfigChanged(); });
         for (auto &scene: eveMng->GetScenes()->RefChildren())
         {
            printf("COMPARE %s %s\n", scene->GetCTitle(), scene->GetCName());
            if (strncmp(scene->GetCTitle(), "Table", 5) == 0)
            {
               scene->AddElement(tablep);
               tableBuilder->Build(rdc, tablep, viewContext );
               printf("build table view");
               break;
            }
         }
         m_builders.push_back(tableBuilder);
      }
      rdc->SetHandlerFunc([&] (REveDataCollection* rdc)
                          {
                             this->CollectionChanged( rdc );
                          });

      rdc->SetHandlerFuncIds([&] (REveDataCollection* rdc, const REveDataCollection::Ids_t& ids)
                             {
                                this->ModelChanged( rdc, ids );
                             });
   }


   void CollectionChanged(REveDataCollection* collection)
   {
      printf("collection changes not implemented %s!\n", collection->GetCName());
   }

   void ModelChanged(REveDataCollection* collection, const REveDataCollection::Ids_t& ids)
   {
      for (auto proxy : m_builders)
      {
         if (proxy->Collection() == collection)
         {
            // printf("Model changes check proxy %s: \n", proxy->Type().c_str());
            proxy->ModelChanges(ids);
         }
      }
   }
};




//==============================================================================
//== Event Manager =============================================================
//==============================================================================

class EventManager : public REveElement
{
private:
   CollectionManager    *m_collectionMng{nullptr};
   nanoaod::Event       *m_event{nullptr};

public:
   EventManager(CollectionManager* m, nanoaod::Event* e):REveElement("EventManager") {m_collectionMng  = m; m_event = e; }
   virtual ~EventManager() {}

   void UpdateTitle()
   {
      SetTitle(Form("%lld/%lld",m_event->GetEvent(), m_event->GetNumEvents() ));
      StampObjProps();
   }
   virtual void NextEvent()
   {
      int id = m_event->GetEvent() +1;
      if (id ==  m_event->GetNumEvents()) {
         printf("NextEvent: reached last %lld\n", m_event->GetNumEvents());
         id = 0;
      }
      m_event->GotoEvent(id);
      UpdateTitle();
      m_collectionMng->RenewEvent();
   }

   virtual void PreviousEvent()
   {
      int id;
      if (m_event->GetEvent() == 0) {
         id = m_event->GetNumEvents()-1;

      }
      else {
          id = m_event->GetEvent() -1;
      }

      printf("going to previous %d \n", id);
      m_event->GotoEvent(id);

      UpdateTitle();
      m_collectionMng->RenewEvent();
   }

   void SetNanoEvent(nanoaod::Event* e) {m_event = e;}
   nanoaod::Event* GetNanoEvent() {return m_event;}
};


//==============================================================================
//== init scenes and views  =============================================================
//==============================================================================
void createScenesAndViews()
{
   //view context
   float r = 300;
   float z = 300;
   auto prop = new REveTrackPropagator();
   prop->SetMagFieldObj(new REveMagFieldDuo(350, -3.5, 2.0));
   prop->SetMaxR(r);
   prop->SetMaxZ(z);
   prop->SetMaxOrbs(6);
   prop->IncRefCount();


   // AMT this is ugly ... introduce a global contenxt
   muonPropagator_g = new REveTrackPropagator();
   muonPropagator_g->SetMagFieldObj(new REveMagFieldDuo(350, -3.5, 2.0));
   muonPropagator_g->SetMaxR(850);
   muonPropagator_g->SetMaxZ(1100);
   muonPropagator_g->SetMaxOrbs(6);
   muonPropagator_g->IncRefCount();

   viewContext = new REveViewContext();
   viewContext->SetBarrel(r, z);
   viewContext->SetTrackPropagator(prop);

   // table specs
   auto tableInfo = new REveTableViewInfo();

   tableInfo->table("nanoaod::Jet").
      column("pt",  1, "i.pt()").
      column("eta", 3, "i.pt()").
      column("phi", 3, "i.phi()");

   tableInfo->table("nanoaod::Electron").
      column("pt",  1, "i.pt()").
      column("eta", 3, "i.pt()").
      column("phi", 3, "i.phi()");


   viewContext->SetTableViewInfo(tableInfo);

   // Geometry
   auto b1 = new REveGeoShape("Barrel 1");
   float dr = 3;
   b1->SetShape(new TGeoTube(290 , 300, 300));
   b1->SetMainColor(kCyan);
   eveMng->GetGlobalScene()->AddElement(b1);

   // Projected RhoZ
   if (1){
      auto rhoZEventScene = eveMng->SpawnNewScene("RhoZ Scene","RhoZProjected");
      mngRhoZ = new REveProjectionManager(REveProjection::kPT_RhoZ);
      mngRhoZ->SetImportEmpty(true);
      auto rhoZView = eveMng->SpawnNewViewer("RhoZ View");
      rhoZView->AddScene(rhoZEventScene);

      auto pgeoScene = eveMng->SpawnNewScene("Projection Geometry");
      mngRhoZ->ImportElements(b1,pgeoScene );
      rhoZView->AddScene(pgeoScene);
   }
      // collections
    auto collections = eveMng->SpawnNewScene("Collections", "Collections");

   // Table
   if (1) {
      auto tableScene = eveMng->SpawnNewScene ("Tables", "Tables");
      auto tableView  = eveMng->SpawnNewViewer("Table", "Table View");
      tableView->AddScene(tableScene);
      tableScene->AddElement(viewContext->GetTableViewInfo());
   }

}

void evd(int portNum=9092)
{
   auto event = g_event;
   event->GotoEvent(0);

   // init REve stuff
   eveMng = REveManager::Create();
   createScenesAndViews();
   auto collectionMng = new CollectionManager(event);

   auto eventMng = new EventManager(collectionMng, event);
   eventMng->UpdateTitle();
   eventMng->SetName(event->GetFile()->GetName());
   eveMng->GetWorld()->AddElement(eventMng);

   // test collections
   collectionMng->addCollection("Jet", kBlue, true);
   collectionMng->addCollection("Electron", kGreen, false);
   collectionMng->addCollection("MET", kRed, false);
   collectionMng->addCollection("Muon", kRed, false);


   std::string locPath = "ui5";
   eveMng->AddLocation("mydir/", locPath);
   eveMng->SetDefaultHtmlPage("file:mydir/eventDisplay.html");

   gEnv->SetValue("WebEve.DisableShow", 1);
   gEnv->SetValue("WebGui.HttpPort", portNum);
   eveMng->Show();
}
