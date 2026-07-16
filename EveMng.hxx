#include "TClass.h"
#include "CmsNanoAod.hxx"
#include "CmsNanoClasses.hxx"


#include "ROOT/REveDataCollection.hxx"
#include "ROOT/REveDataSimpleProxyBuilderTemplate.hxx"
#include "ROOT/REveManager.hxx"
#include "ROOT/REveScalableStraightLineSet.hxx"
#include "ROOT/REveViewContext.hxx"
#include <ROOT/REveGeoShape.hxx>
#include <ROOT/REveJetCone.hxx>
#include <ROOT/REvePointSet.hxx>
#include <ROOT/REveProjectionBases.hxx>
#include <ROOT/REveProjectionManager.hxx>
#include <ROOT/REveScene.hxx>
#include <ROOT/REveTableProxyBuilder.hxx>
#include <ROOT/REveTableInfo.hxx>
#include <ROOT/REveTrack.hxx>
#include <ROOT/REveTrackPropagator.hxx>
#include <ROOT/REveViewer.hxx>
#include <ROOT/REveViewContext.hxx>
#include <ROOT/REveBox.hxx>

#include "TTree.h"
#include "TFile.h"
#include "TEnv.h"
#include "TGeoTube.h"
#include "TList.h"
#include "TParticle.h"
#include "TRandom.h"
#include "TApplication.h"
#include "TMathBase.h"
#include "TRint.h"


using namespace ROOT::Experimental;

//==============================================================================
// == CollectionManager ========================================================
//==============================================================================

class CollectionManager
{
private:
    nanoaod::Event *m_event{nullptr};
    REveElement *m_collections{nullptr};
    std::vector<REveDataProxyBuilderBase *> m_builders;
    bool m_isEventLoading{false}; // don't process model changes when applying filter on new event

public:
    CollectionManager(nanoaod::Event *event);
    void LoadCurrentEventInCollection(REveDataCollection *rdc);
    void RenewEvent();
    void addCollection(REveDataCollection *collection, REveDataProxyBuilderBase *glBuilder);
    void ModelChanged(REveDataItemList *itemList, const REveDataCollection::Ids_t &ids);
    void FillImpliedSelected(REveDataItemList *itemList, REveElement::Set_t &impSelSet, const std::set<int> &sec_idcs);
};

//==============================================================================
//== Event Manager =============================================================
//==============================================================================

class EventManager : public REveElement
{
private:
    CollectionManager *m_collectionMng{nullptr};
    nanoaod::Event *m_event{nullptr};

public:
    EventManager(CollectionManager *m, nanoaod::Event *e);
    virtual ~EventManager() {}
    virtual void GotoEvent(int id);
    void UpdateTitle();
    virtual void NextEvent();
    virtual void PreviousEvent();

    void loadConfig(nlohmann::json&);
    void createScenesAndViews();

void SetNanoEvent(nanoaod::Event *e) { m_event = e; }
nanoaod::Event* GetNanoEvent() { return m_event; }
};