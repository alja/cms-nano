CPPFLAGS := -I$(shell root-config --incdir)
CXXFLAGS := -O0 -g -fPIC $(shell root-config --auxcflags)


ROOTFILE ?= BTag_100.root
CONFIG   ?= cmap.json

# VSDNano provides the event display: managers, views and GUI (ui5/, data/)
VSDNANO  ?= $(HOME)/root-dev/VSDNano

# Synthetic jet tracks (Pythia 8 + FastJet) drawn by JetProxyBuilder, see SynthTracks.h.
# libSynthJetGen.so is built in its own directory: make -C $(SYNTHGEN) libSynthJetGen.so
SYNTHGEN ?= $(HOME)/cms-dev/syntGen
CPPFLAGS += -I$(SYNTHGEN) -I$(VSDNANO)

# Platform linker options. macOS: libraries are found through @rpath and
# there is no --(no-)as-needed.
UNAME ?= $(shell uname -s)
ifeq ($(UNAME),Darwin)
  SONAME       = -Wl,-install_name,@rpath/$@
  EXE_RPATH    = -Wl,-rpath,@loader_path
  NO_AS_NEEDED =
  AS_NEEDED    =
else
  SONAME       =
  EXE_RPATH    = -Wl,-rpath,'$$ORIGIN'
  NO_AS_NEEDED = -Wl,--no-as-needed
  AS_NEEDED    = -Wl,--as-needed
endif

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
	$(CXX) -shared $(SONAME) -o $@ \
	    CmsNanoAod.o CmsNanoAod_dict.o \
	    $(shell root-config --libs)

#===============================================================================
#=================== GEN nanoaod CLASSES =======================================
#===============================================================================

gen-classes: libCmsNanoAod.so
	$(CXX) ${CPPFLAGS} ${CXXFLAGS} -o $@ TestBootstrap.cxx \
		-L. $(EXE_RPATH) \
		-lCmsNanoAod $(shell root-config --libs)

CmsNanoClasses.cxx: gen-classes
	./gen-classes $(ROOTFILE) --fconfig $(CONFIG)

# gen-classes writes the header too
CmsNanoClasses.hxx: CmsNanoClasses.cxx ;


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
	jq -r \
	  '.collections[] | select(has("proxyBuilder")) | "#pragma link C++ class \(.proxyBuilder)-!;"' \
	  $< > $@

EvdGui_dict.cxx: NanoProxyBuilders.h SynthTracks.h $(SYNTHGEN)/SynthJetGen.h CmsNanoClasses.hxx EvdGuiLinkDef.h
	@rm -f EvdGui_dict.cxx
	rootcling -f EvdGui_dict.cxx -I$(SYNTHGEN) -I$(VSDNANO) NanoProxyBuilders.h EvdGuiLinkDef.h


EvdGui_dict.o: EvdGui_dict.cxx
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -c $<

$(SYNTHGEN)/libSynthJetGen.so:
	$(MAKE) -C $(SYNTHGEN) libSynthJetGen.so

libNanoClassesEvd.so: CmsNanoClasses.o CmsNanoClasses_dict.o EvdGui_dict.o libCmsNanoAod.so $(SYNTHGEN)/libSynthJetGen.so
	$(CXX) -shared $(SONAME) -o $@ \
		CmsNanoClasses.o \
		CmsNanoClasses_dict.o \
		EvdGui_dict.o \
		-L. -lCmsNanoAod \
		-L$(SYNTHGEN) -Wl,-rpath,$(SYNTHGEN) -lSynthJetGen \
		-lEG -lGeom -lROOTWebDisplay -lROOTEve \
		$(shell root-config --libs)

$(VSDNANO)/libVsdDict.so $(VSDNANO)/libFWDict.so:
	$(MAKE) -C $(VSDNANO) libVsdDict.so libFWDict.so

evd: evd.cxx NanoProvider.h libNanoClassesEvd.so $(VSDNANO)/libVsdDict.so $(VSDNANO)/libFWDict.so
	$(CXX) $(CPPFLAGS) -I$(VSDNANO) $(CXXFLAGS) -DVSDNANO_DIR='"$(VSDNANO)"' -o $@ \
		evd.cxx \
		-L. $(EXE_RPATH) \
		-L$(VSDNANO) -Wl,-rpath,$(VSDNANO) \
		$(NO_AS_NEEDED) \
		-lNanoClassesEvd \
		-lCmsNanoAod \
		-lVsdDict \
		-lFWDict \
		-lEG -lGeom -lROOTWebDisplay -lROOTEve \
		$(shell root-config --libs)

serviceVSDMama: serviceVSDMama.cc NanoProvider.h libNanoClassesEvd.so $(VSDNANO)/libVsdDict.so $(VSDNANO)/libFWDict.so $(VSDNANO)/FWService.h
	$(CXX) $(CPPFLAGS) -I$(VSDNANO) $(CXXFLAGS) -DVSDNANO_DIR='"$(VSDNANO)"' -o $@ \
		serviceVSDMama.cc \
		-L. $(EXE_RPATH) \
		-L$(VSDNANO) -Wl,-rpath,$(VSDNANO) \
		$(NO_AS_NEEDED) \
		-lNanoClassesEvd \
		-lCmsNanoAod \
		-lVsdDict \
		-lFWDict \
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
	$(CXX) -shared $(SONAME) -o $@ \
	    CmsNanoClasses.o \
	    CmsNanoClasses_dict.o \
	    -L. $(EXE_RPATH) \
	    -lCmsNanoAod \
	    $(shell root-config --libs)

amtprint: TestPrint.o libNanoClassesPrint.so
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ \
	    TestPrint.o \
	    -L. $(EXE_RPATH) \
	    $(NO_AS_NEEDED) \
	    -lNanoClassesPrint \
	    $(AS_NEEDED) \
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
	rm -f evd amtprint serviceVSDMama

cleanevd:
	rm -f CmsNanoClasses.md5 CmsNanoClasses.hxx CmsNanoClasses.cxx
	rm -f nanoClassGen
	rm -f libNanoClassesEvd.so
	rm -f evd