// Event display service for NanoAOD files read through the generated classes
// (nanoaod::MamaCollection), see FWService.h in VSDNano.
//
// A "load" request needs:
//   { "action": "load", "file": "<NanoAOD file>", "config": "<config.json>",
//     "user": "...", "logdir": "...", "logdirurl": "..." }
// "config" is the path of a collection config like cmap.json, or the config
// itself as a JSON object. Collections that are not in the file, or have no
// class in libNanoClassesEvd.so, are left out (see the session log).

#include "NanoProvider.h"
#include "FWService.h"

#ifndef VSDNANO_DIR
#define VSDNANO_DIR "."
#endif

class MamaService : public FWService
{
public:
   MamaService() : FWService("serviceVSDMama", 5556) {}

protected:
   std::string DataFormat(TFile *file) override
   {
      if (!file)
         return "invalid";
      return file->Get("Events") ? "nanoaod" : "invalid";
   }

   FWProvider *CreateProvider(const nlohmann::json &req, std::string &error) override
   {
      std::string dataPath = req["file"].get<std::string>();
      printf("dataPath = %s \n", dataPath.c_str());

      nlohmann::json config;
      if (req["config"].is_object())
      {
         config = req["config"];
      }
      else
      {
         std::string configPath = req["config"].get<std::string>();
         printf("configPath = %s \n", configPath.c_str());
         std::ifstream in(configPath);
         if (!in)
         {
            error = "Can not open config file " + configPath + ".";
            return nullptr;
         }
         in >> config;
      }

      std::unique_ptr<TFile> f(TFile::Open(dataPath.c_str()));
      if (!f || !f->Get("Events"))
      {
         error = "Can not open file " + dataPath + " or it has no Events tree.";
         return nullptr;
      }

      return new NanoProvider(dataPath, config);
   }

   std::string EvdBaseDir() override { return VSDNANO_DIR; }
};

int main(int argc, char *argv[])
{
   MamaService service;
   return service.Main(argc, argv);
}
