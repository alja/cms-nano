#include "NanoProvider.h"

// event display from VSDNano: scenes, views, managers and GUI
#include "evd_main.cc"

#include "TEnv.h"
#include "TRint.h"

#include <fstream>
#include <iostream>

#ifndef VSDNANO_DIR
#define VSDNANO_DIR "."
#endif

int main(int argc, char **argv)
{
   const char *rootFile = nullptr;
   const char *configFile = "cmap.json";  // default

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

   nlohmann::json j;
   in >> j;

   // TApplication writes argv[argc] = nullptr, so leave room for the terminator
   const char *dummyArgvArray[] = {argv[0], nullptr};
   char **dummyArgv = const_cast<char **>(dummyArgvArray);

   int dummyArgc = 1;
   TRint *app = new TRint("evd-test", &dummyArgc, dummyArgv);

   gEnv->SetValue("WebGui.HttpMaxAge", 0);
   gEnv->SetValue("WebGui.HttpPort", 9092);

   auto prov = new NanoProvider(rootFile, j);

   // ui5/ and data/ come from VSDNano
   evd_run(prov, VSDNANO_DIR);

   app->Run();

   return 0;
}
