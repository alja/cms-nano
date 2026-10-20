#ifndef CMS_NANO_AOD_H
#define CMS_NANO_AOD_H

#include "TPRegexp.h"

#include "TLeaf.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

class TTree;
class TFile;

namespace nanoaod
{

class DataMember
{
public:
   std::string  m_name;
   TLeaf       *m_leaf;
   int          m_max_entries;

   // branch-buffer ... if needed ... but this should be automatic

   //---------------------------------------------------------------------------

   DataMember() = delete;

   DataMember(const std::string& name, TLeaf *leaf, int max) :
      m_name        (name),
      m_leaf        (leaf),
      m_max_entries (max)
   {}

   const char* get_name() const { return m_name.c_str();        }
   const char* get_type() const { return m_leaf->GetTypeName(); }

   template<typename T>
   T get_value(int entry) const { return ((T*) m_leaf->GetValuePointer())[entry]; }
};

//==============================================================================

// A member of a generated class: the leaf name (without the collection prefix)
// and the type the class was generated with.
struct SchemaEntry
{
   std::string m_name;
   std::string m_type;
};

class MamaCollection
{
   std::string m_class_name;

   // leaves of this collection found in the current file, in file order
   std::vector<DataMember>    m_members;

   // regexps to catch brach / leaf names
   TPMERegexp  m_num_var_re;
   TPMERegexp  m_data_re;
   int         m_max_ents    = -1; // This means not an array
   int         m_num_datas   =  0;
   TLeaf      *m_num_ents_leaf = nullptr;
   bool        m_present     = false; // any leaf of this collection in the file

   // Members of the generated class, matched to m_members by name, so that a
   // file with a different branch layout than the one the classes were
   // generated from is still read correctly. A slot is an index in m_schema.
   std::vector<SchemaEntry> m_schema;
   std::vector<int>         m_slot_member;  // index in m_members, -1 if not in the file
   std::vector<bool>        m_slot_convert; // leaf type differs, read through TLeaf::GetValue

   // current status
   Long64_t    m_event   = -1;
   int         m_entries = -1;

public:
   // items
   std::function<void* (MamaCollection&, int)>  m_item_ctor;
   std::function<void  (void*)>                 m_item_dtor;
   TClass                                      *m_item_class = nullptr;
   std::vector<void*>                           m_items;

   // Value of member slot for item entry: 0 if the member is not in the file,
   // converted if its type in the file differs from the generated one.
   template<typename T>
   T value(int slot, int entry) const
   {
      int mi = m_slot_member[slot];
      if (mi < 0)
         return T();
      if (m_slot_convert[slot])
         return (T) m_members[mi].m_leaf->GetValue(entry);
      return m_members[mi].get_value<T>(entry);
   }

   // Called by the generated code once the leaves of the file are known.
   void set_schema(std::vector<SchemaEntry> schema);

   //---------------------------------------------------------------------------


   MamaCollection() = delete;

   MamaCollection(const std::string& name);

   ~MamaCollection();

   bool consider_leaf(TLeaf *l);

   void print_class(FILE *fp);
   void print_schema(FILE *fp);

   void goto_event(Long64_t ev);

   // the file has this collection
   bool    is_present()     const { return m_present; }
   // present in the file and compiled in, so it can be shown
   bool    is_usable()      const { return m_present && m_item_class; }

   int     get_n_entries()  const { return m_entries;    }
   void*   get_item(int i)  const { return m_items[i];   }
   TClass* get_item_class() const { return m_item_class; }

   const std::string& get_class_name() const { return m_class_name; }

   template<class T>
   T get_item_with_class(int i) { return T(*this, i); }
};


//==============================================================================

class Event
{
   std::vector<MamaCollection> m_mama_colls;
   std::map<std::string, int>  m_mama_map;

   long long  m_event = -1;

   TFile *m_file = nullptr;
   TTree *m_tree = nullptr;

   bool consider_leaf(TLeaf *l);

   void autogen_nano_classes();
   void autogen_linkdef();

public:
   Event();
   ~Event();

   void RegisterMamaCollection(const std::string& coll_name);

   void OpenFileAndUseTree(const std::string& file_name, const std::string& tree_name="Events");

   void LoadFile(const std::string& file_name, const std::string& tree_name="Events");
   void UseTree(TTree *tree);
   void CloseFile();

   void GotoEvent(Long64_t ev);

   MamaCollection& RefColl(int i) { return m_mama_colls[i]; }

   MamaCollection& RefColl(const std::string& n) { return m_mama_colls[ m_mama_map[n] ]; }

   // null if the collection was not registered
   MamaCollection* FindColl(const std::string& n)
   {
      auto i = m_mama_map.find(n);
      return i == m_mama_map.end() ? nullptr : &m_mama_colls[i->second];
   }

   Long64_t GetEvent() const { return m_event; }

   TFile* GetFile() {return m_file;}

   Long64_t GetNumEvents() const;

   // ----------------------------------------------------------------

   static std::function<void (MamaCollection&)> s_setup_mama_for_type;
};

} // end namespace nanoaod


#endif