// Event display service for NanoAOD files read through the generated classes
// (nanoaod::MamaCollection), see FWService.h in VSDNano.
//
// A "load" request needs:
//   { "action": "load", "file": "<NanoAOD file>", "config": "<config.json>",
//     "user": "...", "logdir": "...", "logdirurl": "..." }
// "config" is the path of a collection config like cmap.json, or the config
// itself as a JSON object. Its collections must be among the ones the classes
// in libNanoClassesEvd.so were generated for.

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

      // the generated classes are compiled in, so the config can only use those
      for (const auto &c : config["collections"])
      {
         std::string cls = "nanoaod::" + c["name"].get<std::string>();
         TClass *tc = TClass::GetClass(cls.c_str());
         if (!tc || !tc->HasDictionary())
         {
            error = "No generated class " + cls + ", rebuild with this config.";
            return nullptr;
         }
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
