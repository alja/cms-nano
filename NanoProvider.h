#ifndef NanoProvider_h
#define NanoProvider_h

#include "FWProvider.h"
#include "CmsNanoAod.hxx"

#include "TClass.h"
#include "TFile.h"
#include "TLeaf.h"
#include "TTree.h"
#include <ROOT/REveTableInfo.hxx>

#include <nlohmann/json.hpp>

// FWProvider for NanoAOD files read through the generated classes.
// The collections come from the config (cmap.json):
//   { "name": "Jet", "proxyBuilder": "nanoaod::JetProxyBuilder<nanoaod::Jet>",
//     "color": 632, "filter": "i.pt() > 20", "var": [ ... ] }
// "proxyBuilder", "color", "filter" and "var" are optional. Collections that
// are not in the data file, or have no compiled-in class, are left out.
class NanoProvider : public FWProvider
{
   nanoaod::Event m_event;
   nlohmann::json m_config;
   std::string    m_title;

   // leaves for the event id, null if the file does not have them
   TLeaf *m_runLeaf{nullptr};
   TLeaf *m_lumiLeaf{nullptr};
   TLeaf *m_eventLeaf{nullptr};

   static double leafValue(TLeaf *l, Long64_t entry)
   {
      if (!l)
         return 0;
      l->GetBranch()->GetEntry(entry);
      return l->GetValue();
   }

public:
   NanoProvider(const std::string &file, const nlohmann::json &config) : m_config(config), m_title(file)
   {
      for (const auto &c : m_config["collections"])
         m_event.RegisterMamaCollection(c["name"]);

      m_event.LoadFile(file);
      m_event.GotoEvent(0);

      if (auto tree = dynamic_cast<TTree *>(m_event.GetFile()->Get("Events")))
      {
         m_runLeaf = tree->GetLeaf("run");
         m_lumiLeaf = tree->GetLeaf("luminosityBlock");
         m_eventLeaf = tree->GetLeaf("event");
      }
   }

   const std::string &GetTitle() const override { return m_title; }

   Long64_t GetNumEvents() override { return m_event.GetNumEvents(); }
   Long64_t GetEventIdx() const override { return m_event.GetEvent(); }
   void     GotoEvent(int eventIdx) override { m_event.GotoEvent(eventIdx); }

   FWEventId GetEventId() override
   {
      Long64_t entry = m_event.GetEvent();
      return FWEventId{UInt_t(leafValue(m_runLeaf, entry)),
                       UInt_t(leafValue(m_lumiLeaf, entry)),
                       Long64_t(leafValue(m_eventLeaf, entry))};
   }

   std::vector<FWCollectionSpec> GetCollectionSpecs() override
   {
      std::vector<FWCollectionSpec> specs;
      for (const auto &c : m_config["collections"])
      {
         std::string name = c["name"];
         if (name == "EventInfo")
            continue;
         if ( ! m_event.RefColl(name).is_usable())
            continue; // LoadFile() printed why

         FWCollectionSpec s;
         s.m_name = name;
         s.m_itemClass = m_event.RefColl(name).get_item_class()->GetName();
         s.m_proxyBuilder = c.value("proxyBuilder", "");
         s.m_color = c.value("color", int(kBlue));
         s.m_filter = c.value("filter", "");
         if (c.contains("var"))
            s.m_varConfig = c["var"].dump();
         specs.push_back(s);
      }
      return specs;
   }

   void GetItems(const std::string &collection, std::vector<void *> &items) override
   {
      nanoaod::MamaCollection &mc = m_event.RefColl(collection);
      for (int i = 0; i < mc.get_n_entries(); ++i)
         items.push_back(mc.get_item(i));
   }

   void SetupTables(ROOT::Experimental::REveTableViewInfo *tableInfo) override
   {
      tableInfo->table("nanoaod::Jet").column("pt", 1, "i.pt()").column("eta", 3, "i.eta()").column("phi", 3, "i.phi()");
      tableInfo->table("nanoaod::Electron").column("pt", 1, "i.pt()").column("eta", 3, "i.eta()").column("phi", 3, "i.phi()");
      tableInfo->table("nanoaod::MET").column("pt", 1, "i.pt()");
      tableInfo->table("nanoaod::PV").column("x", 3, "i.x()").column("y", 3, "i.y()").column("z", 3, "i.z()");
   }
};

#endif
