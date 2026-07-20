#include "EveMng.hxx"
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


#include "proxyBuilders.cxx"

// globals
extern ROOT::Experimental::REveManager* eveMng;
ROOT::Experimental::REveProjectionManager* mngRhoZ;
ROOT::Experimental::REveViewContext* viewContext;
ROOT::Experimental::REveTrackPropagator* muonPropagator_g;

//==============================================================================

//==============================================================================

CollectionManager::CollectionManager(nanoaod::Event *event) : m_event(event)
{
    // collections
    auto collections = eveMng->SpawnNewScene("Collections", "Collections");

    m_collections = eveMng->GetScenes()->FindChild("Collections");
}

void CollectionManager::LoadCurrentEventInCollection(REveDataCollection *rdc)
{
    m_isEventLoading = true;
    rdc->ClearItems();
    nanoaod::MamaCollection &mc = m_event->RefColl(rdc->GetCName());
    std::string cname = rdc->GetName();
    printf("-------- LoadCurrentEventInCollection %s size %d\n", rdc->GetCName(), mc.get_n_entries());
    for (int i = 0; i < mc.get_n_entries(); ++i)
    {
        TString pname(Form("%s %2d", cname.c_str(), i));
        rdc->AddItem(mc.get_item(i), pname.Data(), "");
    }
    rdc->ApplyFilter();
    m_isEventLoading = false;
}

void CollectionManager::RenewEvent()
{
    for (auto &el : m_collections->RefChildren())
    {
        auto c = dynamic_cast<REveDataCollection *>(el);
        LoadCurrentEventInCollection(c);
    }

    for (auto proxy : m_builders)
    {
        proxy->Build();
    }
}

void CollectionManager::addCollection(REveDataCollection *collection, REveDataProxyBuilderBase *glBuilder)
{
    m_collections->AddElement(collection);
    nanoaod::MamaCollection &mc = m_event->RefColl(collection->GetCName());
    //  auto rdc = new REveDataCollection (mc.get_class_name());
    std::cout << "class    !!!" << mc.get_class_name() << std::endl;
    collection->SetItemClass(mc.get_item_class());

    // load data
    LoadCurrentEventInCollection(collection);
    glBuilder->SetCollection(collection);
    glBuilder->SetHaveAWindow(true);

    for (auto &scene : eveMng->GetScenes()->RefChildren())
    {

        REveElement *product = glBuilder->CreateProduct(scene->GetTitle(), viewContext);

        if (strncmp(scene->GetCName(), "Table", 5) == 0)
            continue;
        if (!strncmp(scene->GetCTitle(), "RhoZProjected", 8))
        {
            REveElement *product = glBuilder->CreateProduct("RhoZViewType", viewContext);
            mngRhoZ->ImportElements(product, scene);
            continue;
        }
        else if ((!strncmp(scene->GetCName(), "Event scene", 8)))
        {
            REveElement *product = glBuilder->CreateProduct(scene->GetTitle(), viewContext);
            scene->AddElement(product);
        }
    }
    m_builders.push_back(glBuilder);
    glBuilder->Build();

    // Tables
    auto tableBuilder = new REveTableProxyBuilder();
    tableBuilder->SetHaveAWindow(true);
    tableBuilder->SetCollection(collection);
    REveElement *tablep = tableBuilder->CreateProduct("table-type", viewContext);
    auto tableMng = viewContext->GetTableViewInfo();
    if (collection == m_collections->FirstChild())
    {
        tableMng->SetDisplayedCollection(collection->GetElementId());
    }

    for (auto &scene : eveMng->GetScenes()->RefChildren())
    {
        if (strncmp(scene->GetCTitle(), "Table", 5) == 0)
        {
            scene->AddElement(tablep);
            // tableBuilder->Build(rdc, tablep, viewContext );
            break;
        }
    }
    tableMng->AddDelegate([=]()
                          { tableBuilder->ConfigChanged(); });
    m_builders.push_back(tableBuilder);

    // set tooltip expression for items

    auto tableEntries = tableMng->RefTableEntries(collection->GetItemClass()->GetName());
    int N = TMath::Min(int(tableEntries.size()), 3);
    for (int t = 0; t < N; t++)
    {
        auto te = tableEntries[t];
        collection->GetItemList()->AddTooltipExpression(te.fName, te.fExpression);
    }

    collection->GetItemList()->SetItemsChangeDelegate([&](REveDataItemList *collection, const REveDataCollection::Ids_t &ids)
                                                      { this->ModelChanged(collection, ids); });
    collection->GetItemList()->SetFillImpliedSelectedDelegate([&](REveDataItemList *collection, REveElement::Set_t &impSelSet, const std::set<int> &sec_idcs)
                                                              { this->FillImpliedSelected(collection, impSelSet, sec_idcs); });
}

void CollectionManager::ModelChanged(REveDataItemList *itemList, const REveDataCollection::Ids_t &ids)
{
    if (m_isEventLoading)
        return;

    for (auto proxy : m_builders)
    {
        if (proxy->Collection()->GetItemList() == itemList)
        {
            printf("Model changes check proxy %s: \n", proxy->Collection()->GetCName());
            proxy->ModelChanges(ids);
        }
    }
}
void CollectionManager::FillImpliedSelected(REveDataItemList *itemList, REveElement::Set_t &impSelSet, const std::set<int> &sec_idcs)
{

    for (auto proxy : m_builders)
    {
        if (proxy->Collection()->GetItemList() == itemList)
        {
            proxy->FillImpliedSelected(impSelSet, sec_idcs);
        }
    }
}

//==============================================================================
//== Event Manager =============================================================
//==============================================================================
EventManager::EventManager(CollectionManager *m, nanoaod::Event *e) : REveElement("EventManager")
{
    m_collectionMng = m;
    m_event = e;
    createScenesAndViews();
}

void EventManager::GotoEvent(int id)
{
    m_event->GotoEvent(id);
    UpdateTitle();
    m_collectionMng->RenewEvent();
}

void EventManager::UpdateTitle()
{
    nanoaod::MamaCollection &mc = m_event->RefColl("EventInfo");
    nanoaod::EventInfo ei = mc.get_item_with_class<nanoaod::EventInfo>(0);

    printf("======= update title %lld/%lld \n", m_event->GetEvent(), m_event->GetNumEvents());
    SetTitle(Form("%lld/%lld/%d/%d", m_event->GetEvent(), m_event->GetNumEvents(), ei.run(), ei.luminosityBlock()));
    StampObjProps();
}
void EventManager::NextEvent()
{
    int id = m_event->GetEvent() + 1;
    if (id == m_event->GetNumEvents())
    {
        printf("NextEvent: reached last %lld\n", m_event->GetNumEvents());
        id = 0;
    }
    GotoEvent(id);
}

void EventManager::PreviousEvent()
{
    int id;
    if (m_event->GetEvent() == 0)
    {
        id = m_event->GetNumEvents() - 1;
    }
    else
    {
        id = m_event->GetEvent() - 1;
    }

    printf("going to previous %d \n", id);
    GotoEvent(id);
}

void EventManager::loadConfig(nlohmann::json& j)
{
   for (const auto &c : j["collections"])
   {
    std::string cname = c["name"];
     REveDataCollection *eveCol = new REveDataCollection(cname.c_str());
     eveCol->SetMainColor(c["color"]);

     std::string pbName = c["proxyBuilder"];
     TClass *cl = TClass::GetClass(pbName.c_str());
     REveDataProxyBuilderBase* pb = (REveDataProxyBuilderBase*)cl->New();
     m_collectionMng->addCollection(eveCol, pb);
   }
}

void EventManager::createScenesAndViews()
{
    // view context
    float r = 139.5;
    float z = 290;
    auto prop = new REveTrackPropagator();
    prop->SetMagFieldObj(new REveMagFieldDuo(350, -3.5, 2.0));
    prop->SetMaxR(r);
    prop->SetMaxZ(z);
    prop->SetMaxOrbs(6);
    prop->IncRefCount();

    // AMT this is ugly ... introduce a global contenxt
    muonPropagator_g = new REveTrackPropagator();
    muonPropagator_g->SetMagFieldObj(new REveMagFieldDuo(350, -3.5, 2.0));
    muonPropagator_g->SetMaxR(850);
    muonPropagator_g->SetMaxZ(1100);
    muonPropagator_g->SetMaxOrbs(6);
    muonPropagator_g->IncRefCount();

    viewContext = new REveViewContext();
    viewContext->SetBarrel(r, z);
    viewContext->SetTrackPropagator(prop);

    // table specs
    auto tableInfo = new REveTableViewInfo();

    tableInfo->table("nanoaod::Jet").column("pt", 1, "i.pt()").column("eta", 3, "i.eta()").column("phi", 3, "i.phi()");

    tableInfo->table("nanoaod::Electron").column("pt", 1, "i.pt()").column("eta", 3, "i.eta()").column("phi", 3, "i.phi()");

    tableInfo->table("nanoaod::MET").column("pt", 1, "i.pt()");

    viewContext->SetTableViewInfo(tableInfo);

    // Geom  ry
    auto b1 = new REveGeoShape("Barrel 1");
    float dr = 3;
    b1->SetShape(new TGeoTube(r - 2, r + 2, z));
    b1->SetMainColor(kCyan);
    eveMng->GetGlobalScene()->AddElement(b1);

    // Projected RhoZ
    if (1)
    {
        auto rhoZEventScene = eveMng->SpawnNewScene("RhoZ Scene", "RhoZProjected");
        mngRhoZ = new REveProjectionManager(REveProjection::kPT_RhoZ);
        mngRhoZ->SetImportEmpty(true);
        auto rhoZView = eveMng->SpawnNewViewer("RhoZ View");
        rhoZView->SetCameraType(REveViewer::kCameraOrthoXOY);
        rhoZView->AddScene(rhoZEventScene);

        auto pgeoScene = eveMng->SpawnNewScene("Projection Geometry");
        mngRhoZ->ImportElements(b1, pgeoScene);
        rhoZView->AddScene(pgeoScene);
    }

    // Table
    if (1)
    {
        auto tableScene = eveMng->SpawnNewScene("Tables", "Tables");
        auto tableView = eveMng->SpawnNewViewer("Table", "Table View");
        tableView->AddScene(tableScene);
        tableScene->AddElement(viewContext->GetTableViewInfo());
    }
}

