CPPFLAGS := -I$(shell root-config --incdir)
CXXFLAGS := -O0 -g -fPIC $(shell root-config --auxcflags)


ROOTFILE ?= BTag.root
CONFIG   ?= cmap.json

#===============================================================================
#================= CORE ========================================================
#===============================================================================
CmsNanoAod.o: CmsNanoAod.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -c $<


CmsNanoAod_dict.cxx: CmsNanoAod.hxx
	@rm -f CmsNanoAod_dict.cxx
	rootcling -f CmsNanoAod_dict.cxx CmsNanoAod.hxx CmsNanoAod_LinkDef.h

CmsNanoAod_dict.o: CmsNanoAod_dict.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -c $<

libCmsNanoAod.so: CmsNanoAod.o CmsNanoAod_dict.o
	$(CXX) -shared -o $@ \
	    CmsNanoAod.o CmsNanoAod_dict.o \
	    $(shell root-config --libs)

#===============================================================================
#=================== GEN nanoaod CLASSES =======================================
#===============================================================================

gen-classes: libCmsNanoAod.so
	$(CXX) ${CPPFLAGS} ${CXXFLAGS} -o $@ TestBootstrap.cxx \
		-L. -Wl,-rpath,'$$ORIGIN' \
		-lCmsNanoAod $(shell root-config --libs)

CmsNanoClasses.cxx: gen-classes
	./gen-classes $(ROOTFILE) --fconfig $(CONFIG)


CmsNanoClasses_dict.cxx: CmsNanoClasses.hxx
	@rm -f CmsNanoClasses_dict.cxx
	rootcling -f CmsNanoClasses_dict.cxx CmsNanoClasses.hxx EvdNanoClassesLinkDef.h


CmsNanoClasses.o: CmsNanoClasses.cxx CmsNanoClasses.hxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -c $<

CmsNanoClasses_dict.o: CmsNanoClasses_dict.cxx CmsNanoClasses.hxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -c $<

#===============================================================================
#=================== VISUALIZATION  ============================================
#===============================================================================

EvdGuiLinkDef.h: $(CONFIG)
	@echo '#pragma link C++ class EventManager-!;' > $@
	jq -r \
	  '.collections[] | select(has("proxyBuilder")) | "#pragma link C++ class \(.proxyBuilder)-!;"' \
	  $< >> $@

EvdGui_dict.cxx: EveMng.hxx EvdGuiLinkDef.h
	@rm -f EvdGui_dict.cxx
	rootcling -f EvdGui_dict.cxx EveMng.hxx proxyBuilders.cxx EvdGuiLinkDef.h


EvdGui_dict.o: EvdGui_dict.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -c $<

libNanoClassesEvd.so: CmsNanoClasses.o CmsNanoClasses_dict.o EvdGui_dict.o libCmsNanoAod.so
	$(CXX) -shared -o $@ \
		CmsNanoClasses.o \
		CmsNanoClasses_dict.o \
		EvdGui_dict.o \
		-L. -lCmsNanoAod \
		$(shell root-config --libs)

evd: libNanoClassesEvd.so
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ \
		evd.cxx EveMng.cxx \
		-L. -Wl,-rpath,'$$ORIGIN' \
		-lNanoClassesEvd \
		-lCmsNanoAod \
		-lEG -lGeom -lROOTWebDisplay -lROOTEve \
		$(shell root-config --libs)

display: evd
	./evd $(ROOTFILE) --fconfig $(CONFIG)


#=================================================================================
#========= print test ============================================================
#=================================================================================

TestPrint.o: TestPrint.cxx
	$(CXX) ${CPPFLAGS} ${CXXFLAGS} -c $<


libNanoClassesPrint.so: CmsNanoClasses.o CmsNanoClasses_dict.o libCmsNanoAod.so
	$(CXX) -shared -o $@ \
	    CmsNanoClasses.o \
	    CmsNanoClasses_dict.o \
	    -L. -Wl,-rpath,'$$ORIGIN' \
	    -lCmsNanoAod \
	    $(shell root-config --libs)

amtprint: TestPrint.o libNanoClassesPrint.so
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ \
	    TestPrint.o \
	    -L. -Wl,-rpath,'$$ORIGIN' \
	    -Wl,--no-as-needed \
	    -lNanoClassesPrint \
	    -Wl,--as-needed \
	    -lCmsNanoAod \
	    $(shell root-config --libs)

#===============================================================================

clean:
	rm -rf EvdGuiLinkDef.h
	rm -rf libCmsNanoAod.so CmsNanoAod_dict* CmsNanoAod.o
	rm -rf CmsNanoAod_cxx.d CmsNanoAod_cxx_ACLiC_dict_rdict.pcm CmsNanoAod_cxx.so
	rm -rf CmsNanoClasses.md5 CmsNanoClasses.hxx CmsNanoClasses.cxx CmsNanoClasses.o
	rm -f TestBootstrap.o TestPrint.o
	rm -f gen-classes
	rm -f CmsNanoClasses_dict.cxx CmsNanoClasses_dict.o CmsNanoClasses_dict_rdict.pcm
	rm -f EvdGui_dict.cxx EvdGui_dict.o EvdGui_dict_rdict.pcm
	rm -f libNanoClassesEvd.so
	rm -f libNanoClassesPrint.so
	rm -f evd amtprint

cleanevd:
	rm -f CmsNanoClasses.md5 CmsNanoClasses.hxx CmsNanoClasses.cxx
	rm -f nanoClassGen
	rm -f libNanoClassesEvd.so
	rm -f evd