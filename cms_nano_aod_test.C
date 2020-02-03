// Run the whole thing as:
//   root.exe CmsNanoAod.cxx+ cms_nano_aod_bootstrap.C cms_nano_aod_test.C
//
// - CmsNanoAod.hxx/cxx - contain the real code.
//
// - cms_nano_aod_bootstrap.C - makes sure the NANO structs get created
//   and loaded; this needs to be done before we can access
//   them as types. REve might be able to get by without that
//   is it only uses names and TClass info.
//
// - cms_nano_aod_test.C - this file, minimal test.
//
// As part of bootstrap nanoaod_structs_auto.C gets created and
// loaded. nanoaod_structs_auto.md5 contains MD5 of TLeaf
// names and types of that file so we can detect if the
// NANO contents has changed (and so nanoaod_structs_auto.C needs
// to be recreated).

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

void cms_nano_aod_test()
{
   auto event = g_event;

   nanoaod::MamaCollection& els = event->RefColl("Electron");
   nano_view(event);

   {
      float mpt1, mpt2;
      mpt1 = event->RefColl("MET").get_item_with_class<nanoaod::MET>(0).pt();
      {
         using namespace nanoaod;
         mpt2 = event->RefColl("MET").get_item_with_class<MET>(0).pt();
      }

      printf("\nE.g. getting MET for the last event in two ways: %f and %f\n",
             mpt1, mpt2);
   }
}
