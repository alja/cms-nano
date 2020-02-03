#include "TPRegexp.h"

#include "TLeaf.h"

#include <functional>

class TTree;
class TFile;


#ifdef __ROOTCLING__
#pragma link C++ class nanoaod::DataMember-!;
#pragma link C++ class nanoaod::MamaCollection-!;
#pragma link C++ class nanoaod::Event-!;
#endif

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

class MamaCollection
{
   std::string m_class_name;

   std::vector<DataMember>    m_members;

   // regexps to catch brach / leaf names
   TPMERegexp  m_num_var_re;
   TPMERegexp  m_data_re;
   int         m_max_ents    = -1; // This means not an array
   int         m_num_datas   =  0;
   TLeaf      *m_num_ents_leaf;

   // current status
   Long64_t    m_event   = -1;
   int         m_entries = -1;

public:
   // items
   std::function<void* (MamaCollection&, int)>  m_item_ctor;
   std::function<void  (void*)>                 m_item_dtor;
   TClass                                      *m_item_class = nullptr;
   std::vector<void*>                           m_items;

   DataMember& data_member(int i) { return m_members[i]; }

   //---------------------------------------------------------------------------


   MamaCollection() = delete;

   MamaCollection(const std::string& name);

   ~MamaCollection();

   bool consider_leaf(TLeaf *l);

   void print_class(FILE *fp);

   void goto_event(Long64_t ev);

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

public:
   Event();
   ~Event();

   void RegisterMamaCollection(const std::string& coll_name);

   void OpenFileAndUseTree(const std::string& file_name, const std::string& tree_name="Events");
   void UseTree(TTree *tree);
   void CloseFile();

   void GotoEvent(Long64_t ev);

   MamaCollection& RefColl(int i) { return m_mama_colls[i]; }

   MamaCollection& RefColl(const std::string& n) { return m_mama_colls[ m_mama_map[n] ]; }

   Long64_t GetEvent() const { return m_event; }

   TFile* GetFile() {return m_file;}

   Long64_t GetNumEvents() const;

   // ----------------------------------------------------------------

   static std::function<void (MamaCollection&)> s_setup_mama_for_type;
};

} // end namespace nanoaod
