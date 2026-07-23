
#include "TestPrint.h"
#include "TClass.h"
#include "CmsNanoAod.hxx"
#include "CmsNanoClasses.hxx"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

int main(int argc, char **argv)
{
   printf("Print teset amt \n");
   using json = nlohmann::json;
   using namespace nanoaod;

   const char *rootFile = nullptr;
   const char *configFile = "cmap.json"; // default

   if (argc > 1)
      rootFile = argv[1];
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
         configFile = argv[++i];
      }
      else
      {
         std::cerr << "Unknown option: " << arg << "\n";
         return 1;
      }
   }

   std::ifstream in(configFile);
   if (!in)
   {
      std::cerr << "Failed to open config file: " << configFile << "\n";
      return 1;
   }

   json j;
   in >> j;

   auto event = new nanoaod::Event();

   for (const auto &c : j["collections"])
      event->RegisterMamaCollection(c["name"]);

   event->LoadFile(rootFile);

   
for (int ev = 0; ev < 10; ++ev)
   {
      event->GotoEvent(ev);

      for (const auto &c : j["collections"])
      {
         std::string cname = c["name"];

         MamaCollection &els = event->RefColl(cname.c_str());
         int n_els = els.get_n_entries();
         printf("Event %d, %s n_itmes=%d\n", ev, cname.c_str(), n_els);
      }
   }
   return 0;
}
