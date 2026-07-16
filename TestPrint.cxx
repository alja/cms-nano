
#include "TestPrint.h"
#include "TClass.h"
#include "CmsNanoAod.hxx"
#include "CmsNanoClasses.hxx"


void nano_view(nanoaod::Event *event)
{
   using namespace nanoaod;

   MamaCollection& els = event->RefColl("Electron");

   for (int ev = 0; ev < 10; ++ev)
   {
      event->GotoEvent(ev);

      int n_els = els.get_n_entries();

      printf("Event %d, n_els=%d\n", ev, n_els);

      for (int i = 0; i < n_els; ++i)
      {
         Electron el = els.get_item_with_class<Electron>(i);

         printf("  %d %f %f %f\n", i, el.pt(), el.eta(), el.phi());
      }
   }
}


int main(int argc, char **argv)
{
   printf("Print teset amt \n");

   auto event = new nanoaod::Event();
   event->RegisterMamaCollection("Electron");
   event->OpenFileAndUseTree(
      "nano-CMSSW_11_0_0-RelValZTT-mcRun.root");
   nano_view(event);

   return 0;
}
