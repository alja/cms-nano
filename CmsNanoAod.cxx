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
      return true;
   }

   if (m_data_re.Match(s) == 2)
   {
      // printf("Matched data %s for %s\n", m_data_re[1].Data(), m_class_name.c_str());

      m_members.push_back( DataMember(m_data_re[1].Data(), l, m_max_ents) );

      ++m_num_datas;

      return true;
   }

   return false;
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

   int mi = 0;
   for (auto &m : m_members)
   {
      fprintf(fp, "   %s %s() { return m_mama.data_member(%d).get_value<%s>(m_seq_idx); }\n",
              m.get_type(), m.get_name(), mi, m.get_type());
      ++mi;

   }
   fprintf(fp, "};\n");
}

void MamaCollection::goto_event(Long64_t ev)
{
   m_event = ev;

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

void Event::autogen_nano_classes()
{
   FILE *fp = fopen("nano_mama.C", "w");

   fprintf(fp, "namespace nanoaod\n{\n\n");

   for (auto& mc : m_mama_colls)
   {
      mc.print_class(fp);
      fprintf(fp, "\n");
   }

   fprintf(fp, "//========================================================================\n\n");

   fprintf(fp, "struct SetterUpper { SetterUpper() { Event::s_setup_mama_for_type = [](MamaCollection& mc)\n{\n  ");

   for (auto& mc : m_mama_colls)
   {
      const char *cn = mc.get_class_name().c_str();

      fprintf(fp,
              " if (mc.get_class_name() == \"%s\")\n"
              "   {\n"
              "      mc.m_item_ctor  = [](MamaCollection& m, int idx) { return new %s(m, idx); };\n"
              "      mc.m_item_dtor  = [](void* item) { delete (%s*) item; };\n"
              "      mc.m_item_class = TClass::GetClass<%s>();\n"
              "   }\n"
              "   else",
              cn, cn, cn, cn
              );
   }
   fprintf(fp, "\n   {\n      throw std::runtime_error(mc.get_class_name() + \" unknown class name\");\n   }\n");

   fprintf(fp, "};} } setter_upper;\n\n");

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

   TMD5 *md5_on_file = TMD5::ReadChecksum("nano_mama.md5");

   if ( ! md5_on_file || strcmp(md5.AsString(), md5_on_file->AsString()) != 0)
   {
      printf("Regenerating nano classes ...\n");

      autogen_nano_classes();
      TMD5::WriteChecksum("nano_mama.md5", &md5);
   }
   else
   {
      printf("Reusing existing nano classes ...\n");
   }

   delete md5_on_file;

   gROOT->ProcessLine(".L nano_mama.C");

   for (auto& mc : m_mama_colls)
   {
      s_setup_mama_for_type(mc);
   }
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
