#include "TClass.h"
#include "CmsNanoClasses.hxx"


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

#include "TTree.h"
#include "TFile.h"
#include "TEnv.h"
#include "TGeoTube.h"
#include "TList.h"
#include "TParticle.h"
#include "TRandom.h"
#include "TApplication.h"
#include "TMathBase.h"
#include "TRint.h"

#include "EveMng.hxx"

// globals
ROOT::Experimental::REveManager* eveMng;

using namespace ROOT::Experimental;

int main(int argc, char **argv)
{

   const char *dummyArgvArray[] = {argv[0]};
   char **dummyArgv = const_cast<char **>(dummyArgvArray);
   int dummyArgc = 1;
   TRint *app = new TRint("evd-test", &dummyArgc, dummyArgv);

   int portNum = 9092;

   // init nanoaod stuff
   gSystem->Load("libNanoClassesEvd.so");
   auto event = new nanoaod::Event();
   event->RegisterMamaCollection("EventInfo");

   event->RegisterMamaCollection("Electron");
   event->RegisterMamaCollection("Jet");
   event->RegisterMamaCollection("MET");
   event->RegisterMamaCollection("Muon");

   event->LoadFile("nano-CMSSW_11_0_0-RelValZTT-mcRun.root");
   event->GotoEvent(0);

   // init REve stuff
   eveMng = REveManager::Create();
   eveMng->AllowMultipleRemoteConnections(false, false);
   // createScenesAndViews();
   auto collectionMng = new CollectionManager(event);

   auto eventMng = new EventManager(collectionMng, event);
   eventMng->UpdateTitle();
   eventMng->SetName(event->GetFile()->GetName());

   eventMng->dummyTest();

   eventMng->GotoEvent(0);
   eveMng->GetWorld()->AddElement(eventMng);

   std::string locPath = "ui5";
   eveMng->AddLocation("mydir/", locPath);
   eveMng->SetDefaultHtmlPage("file:mydir/eventDisplay.html");

   gEnv->SetValue("WebEve.DisableShow", 1);
   gEnv->SetValue("WebGui.HttpMaxAge", 0);
   gEnv->SetValue("WebGui.HttpPort", portNum);
   eveMng->Show();
   app->Run();

   return 0;
}
