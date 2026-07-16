#include "TestBootstrap.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

nanoaod::Event* g_event = nullptr;

int main(int argc, char **argv)
{
   const char *filename = (argc > 1) ? argv[1] : "cmap.json";
   std::ifstream in(filename);
   if (!in)
   {
      std::cerr << "Failed to open file: " << filename << "\n";
      return 1;
   }

   json j;
   in >> j;

   g_event = new nanoaod::Event();

   g_event->RegisterMamaCollection("EventInfo");
   for (const auto &c : j["collections"])
      g_event->RegisterMamaCollection(c["name"]);
   
   g_event->OpenFileAndUseTree(
       "nano-CMSSW_11_0_0-RelValZTT-mcRun.root");
   return 0;
}