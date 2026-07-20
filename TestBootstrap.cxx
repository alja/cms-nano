#include "TestBootstrap.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

nanoaod::Event* g_event = nullptr;

/*
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
   */
  
int main(int argc, char **argv)
{
   const char *filename = nullptr;
   const char *configfile = "cmap.json";  // default

   if (argc > 1)
      filename = argv[1];
   else
   {
      std::cerr << "Usage: " << argv[0]
                << " <root-file> [--fconfig <config.json>]\n";
      return 1;
   }

   // parse options
   for (int i = 2; i < argc; ++i)
   {
      std::string arg = argv[i];

      if (arg == "--fconfig")
      {
         if (i + 1 >= argc)
         {
            std::cerr << "--fconfig requires a file name\n";
            return 1;
         }
         configfile = argv[++i];
      }
      else
      {
         std::cerr << "Unknown option: " << arg << "\n";
         return 1;
      }
   }

   std::ifstream in(configfile);
   if (!in)
   {
      std::cerr << "Failed to open config file: " << configfile << "\n";
      return 1;
   }

   json j;
   in >> j;

   g_event = new nanoaod::Event();

   g_event->RegisterMamaCollection("EventInfo");

   for (const auto &c : j["collections"])
      g_event->RegisterMamaCollection(c["name"]);

   g_event->OpenFileAndUseTree(filename);

   return 0;
}