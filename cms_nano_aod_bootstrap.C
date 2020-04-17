nanoaod::Event * g_event = nullptr;

void cms_nano_aod_config_event(nanoaod::Event *e)
{
   // EventInfo is handled specifically to pick up run / lumi / event ids.
   e->RegisterMamaCollection("EventInfo");

   e->RegisterMamaCollection("Electron");
   e->RegisterMamaCollection("Jet");
   e->RegisterMamaCollection("MET");
   e->RegisterMamaCollection("Muon");
}

void cms_nano_aod_bootstrap()
{
   g_event = new nanoaod::Event();

   cms_nano_aod_config_event(g_event);

   //  g_event->OpenFileAndUseTree("nano-CMSSW_11_0_0-RelValZTT-mcRun.root");
   g_event->OpenFileAndUseTree("nano-CMSSW_11_0_0-RelValZTT-mcRun.root");
}
