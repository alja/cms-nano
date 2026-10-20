#include "CmsNanoAod.hxx"

#include "TROOT.h"

#include "TBranch.h"
#include "TTree.h"
#include "TFile.h"
#include "TMD5.h"

#include <cstring>

namespace nanoaod
{

//==============================================================================
// MamaCollection
//==============================================================================

MamaCollection::MamaCollection(const std::string& name) :
      m_class_name (name)
   {
      TString s;
      s = "^n"; s += name; s += "$";
      m_num_var_re.Reset(s, "o", 1);

      s = "^"; s += name; s += "_(.*)$";
      m_data_re.Reset(s, "o", 1);

      if (name == "EventInfo")
      {
         m_data_re.Reset("^(run|luminosityBlock|event)$", "o", 1);
      }
   }

MamaCollection::~MamaCollection()
{
   for (auto& di : m_items)
   {
      m_item_dtor(di);
   }
}

bool MamaCollection::consider_leaf(TLeaf *l)
{
   TString s(l->GetName());

   if (m_num_var_re.Match(s) == 1)
   {
      // printf("Matched num %s for %s\n", m_num_var_re[0].Data(), m_class_name.c_str());

      m_num_ents_leaf = l;
      m_max_ents      = l->GetMaximum();
      m_present       = true;
      return true;
   }

   if (m_data_re.Match(s) == 2)
   {
      // printf("Matched data %s for %s\n", m_data_re[1].Data(), m_class_name.c_str());

      m_members.push_back( DataMember(m_data_re[1].Data(), l, m_max_ents) );

      ++m_num_datas;
      m_present = true;

      return true;
   }

   return false;
}

void MamaCollection::set_schema(std::vector<SchemaEntry> schema)
{
   m_schema = std::move(schema);
   m_slot_member.assign(m_schema.size(), -1);
   m_slot_convert.assign(m_schema.size(), false);

   std::map<std::string, int> by_name;
   for (int i = 0; i < (int) m_members.size(); ++i)
      by_name[m_members[i].m_name] = i;

   int n_missing = 0, n_converted = 0;
   for (int s = 0; s < (int) m_schema.size(); ++s)
   {
      auto it = by_name.find(m_schema[s].m_name);
      if (it == by_name.end())
      {
         ++n_missing;
         continue;
      }
      m_slot_member[s] = it->second;
      if (m_schema[s].m_type != m_members[it->second].get_type())
      {
         m_slot_convert[s] = true;
         ++n_converted;
      }
   }

   if (n_missing || n_converted)
      printf("Collection %s: %d of %d members not in this file (read as 0), %d with a different type (converted).\n",
             m_class_name.c_str(), n_missing, (int) m_schema.size(), n_converted);
}

void MamaCollection::print_class(FILE *fp)
{
   fprintf(fp, "class %s\n", m_class_name.c_str());
   fprintf(fp, "{\npublic:\n");
   fprintf(fp,
           "   nanoaod::MamaCollection &m_mama;\n"
           "   int             m_seq_idx;\n"
           "\n"
           "   %s(nanoaod::MamaCollection &mc, int idx) : m_mama(mc), m_seq_idx(idx) {}\n"
           "\n",
           m_class_name.c_str()
           );

   // Members are read through slots, matched by name to the leaves of the
   // file that is opened (see MamaCollection::set_schema).
   int mi = 0;
   for (auto &m : m_members)
   {
      fprintf(fp, "   %s %s() const { return m_mama.value<%s>(%d, m_seq_idx); }\n",
              m.get_type(), m.get_name(), m.get_type(), mi);
      ++mi;

   }
   fprintf(fp, "};\n");
}

// Member names and types in slot order, as initializer list entries.
void MamaCollection::print_schema(FILE *fp)
{
   bool first = true;
   for (auto &m : m_members)
   {
      fprintf(fp, "%s{\"%s\", \"%s\"}", first ? "" : ", ", m.get_name(), m.get_type());
      first = false;
   }
}

void MamaCollection::goto_event(Long64_t ev)
{
   m_event = ev;

   // not in this file or not compiled in: no items
   if ( ! is_usable())
   {
      m_entries = 0;
      return;
   }

   if (m_max_ents > -1)
   {
      m_num_ents_leaf->GetBranch()->GetEntry(ev);
      m_entries = ((UInt_t*)(m_num_ents_leaf->GetValuePointer()))[0];
   }
   else
   {
      // If not a vector (does not have nClass branch), assume single entry.
      m_entries = 1;
   }

   for (auto &m : m_members)
   {
      m.m_leaf->GetBranch()->GetEntry(ev);
   }

   if ((int) m_items.size() < m_entries)
   {
      int i = m_items.size();
      m_items.resize(m_entries);
      while (i < m_entries)
      {
         m_items[i] = m_item_ctor(*this, i);
         ++i;
      }
   }
}


//==============================================================================
// Event
//==============================================================================

std::function<void (MamaCollection&)> Event::s_setup_mama_for_type;

Event::Event()
{}


Event::~Event()
{
   if (m_file) CloseFile();
}

//------------------------------------------------------------------------------

bool Event::consider_leaf(TLeaf *l)
{
   for (auto& mc : m_mama_colls)
   {
      if (mc.consider_leaf(l))
         return true;
   }

   return false;
}

void Event::autogen_linkdef()
{

   FILE *fp = fopen("EvdNanoClassesLinkDef.h", "w");
   for (auto& mc : m_mama_colls)
   {
      if ( ! mc.is_present()) continue;
      const char *cn = mc.get_class_name().c_str();
      fprintf(fp, "#pragma link C++ class nanoaod::%s-!;\n", cn);
   }

   fclose(fp);
}

void Event::autogen_nano_classes()
{
   FILE *fp = fopen("CmsNanoClasses.hxx", "w");
   fprintf(fp, "#ifndef CLASSES_BOOTSTRAP_H\n");
   fprintf(fp, "#define CLASSES_BOOTSTRAP_H\n");

   fprintf(fp, "#include \"TClass.h\"\n");
   fprintf(fp, "#include \"CmsNanoAod.hxx\"\n");
   fprintf(fp, "namespace nanoaod\n{\n\n");

   for (auto& mc : m_mama_colls)
   {
      if ( ! mc.is_present()) continue;
      mc.print_class(fp);
      fprintf(fp, "\n");
   }

   fprintf(fp, "} // end namespace nanoaod\n");

   fprintf(fp,"#endif\n");

   fclose(fp);

   //========================================================================


   fp = fopen("CmsNanoClasses.cxx", "w");

   fprintf(fp, "#include \"CmsNanoClasses.hxx\"\n");
   fprintf(fp, "#include <iostream>\n");
   fprintf(fp, "namespace nanoaod\n{\n\n");

   fprintf(fp, "struct SetterUpper {\n"
               " SetterUpper() {\n"
               "  Event::s_setup_mama_for_type = [](MamaCollection& mc) {\n  ");

   for (auto& mc : m_mama_colls)
   {
      if ( ! mc.is_present()) continue;
      const char *cn = mc.get_class_name().c_str();

      fprintf(fp,
              "   if (mc.get_class_name() == \"%s\")\n"
              "   {\n"
              "      std::cerr << \"setup called for \" << mc.get_class_name() << std::endl;  \n "
              "      mc.m_item_ctor  = [](MamaCollection& m, int idx) { return new %s(m, idx); };\n"
              "      mc.m_item_dtor  = [](void* item) { delete (%s*) item; };\n"
              "      mc.m_item_class = TClass::GetClass<%s>();\n"
              "      mc.set_schema({",
              cn, cn, cn, cn
              );
      mc.print_schema(fp);
      fprintf(fp, "});\n"
                  "   }\n"
                  "   else");
   }
   fprintf(fp, "\n   {\n      throw std::runtime_error(mc.get_class_name() + \" unknown class name\");\n   }\n");

   fprintf(fp, "  };\n   }\n} setter_upper;\n\n");

   fprintf(fp, "} // end namespace nanoaod\n");


   fclose(fp);
}

//------------------------------------------------------------------------------

void Event::OpenFileAndUseTree(const std::string& file_name, const std::string& tree_name)
{
   m_file = TFile::Open(file_name.c_str());

   if (m_file == nullptr || m_file->IsZombie()) throw std::runtime_error("file opening root file");

   TTree *tree = (TTree*) m_file->Get("Events");

   if (tree == nullptr) throw std::runtime_error("fail getting Events tree");

   UseTree(tree);
}

void Event::UseTree(TTree *tree)
{
   m_tree = tree;

   TObjArray *leaves   = m_tree->GetListOfLeaves();
   Int_t      n_leaves = leaves ? leaves->GetEntriesFast() : 0;

   TMD5 md5;

   for (int l = 0; l < n_leaves; ++l)
   {
      TLeaf *leaf = (TLeaf*) leaves->UncheckedAt(l);

      md5.Update((const UChar_t*) leaf->GetName(),     strlen(leaf->GetName()));
      md5.Update((const UChar_t*) leaf->GetTypeName(), strlen(leaf->GetTypeName()));
      // printf("trying out leaf %s, type %s\n", leaf->GetName(), leaf->GetTypeName());

      consider_leaf(leaf);
   }
   md5.Final();

   for (auto& mc : m_mama_colls)
   {
      if ( ! mc.is_present())
         printf("Collection %s is not in this file, no class is generated for it.\n", mc.get_class_name().c_str());
   }

   // The Makefile reruns this only when the data file or the config change,
   // so always regenerate. The checksum of the leaf layout is kept for reference.
   printf("Generating nano classes ...\n");

   autogen_linkdef();
   autogen_nano_classes();
   TMD5::WriteChecksum("CmsNanoClasses.md5", &md5);

}

void Event::CloseFile()
{
   if (m_file)
   {
      if (m_tree) delete m_tree;

      m_file->Close();
      delete m_file;
      m_file = 0;
   }
   
   m_tree = 0;
}

void Event::LoadFile(const std::string& file_name,  const std::string& tree_name)
{
   m_file = TFile::Open(file_name.c_str());

   if (m_file == nullptr || m_file->IsZombie()) throw std::runtime_error("file opening root file");

   m_tree = (TTree*) m_file->Get("Events");

   TObjArray *leaves   = m_tree->GetListOfLeaves();
   Int_t      n_leaves = leaves ? leaves->GetEntriesFast() : 0;
   for (int l = 0; l < n_leaves; ++l)
   {
      TLeaf *leaf = (TLeaf*) leaves->UncheckedAt(l);
      consider_leaf(leaf);
   }

   for (auto& mc : m_mama_colls)
   {
      if ( ! mc.is_present())
      {
         printf("Collection %s is not in this file, it is not shown.\n", mc.get_class_name().c_str());
         continue;
      }
      try
      {
         s_setup_mama_for_type(mc);
      }
      catch (std::exception &)
      {
         printf("Collection %s is not in the compiled classes, it is not shown. "
                "Rebuild with a data file that has it, e.g. make evd ROOTFILE=<file>.\n",
                mc.get_class_name().c_str());
      }
   }
}

//------------------------------------------------------------------------------

void Event::RegisterMamaCollection(const std::string& coll_name)
{
   m_mama_map[coll_name] = m_mama_colls.size();

   m_mama_colls.push_back( MamaCollection(coll_name) );
}

void Event::GotoEvent(Long64_t ev)
{
   m_event = ev;

   for (auto& mc : m_mama_colls)
   {
      mc.goto_event(ev);
   }
}

Long64_t Event::GetNumEvents() const
{
   return m_tree ? m_tree->GetEntriesFast() : 0; 
}
} // end namespace nanoaod

void CmsNanoAod() {}
