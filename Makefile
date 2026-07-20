CPPFLAGS := -I$(shell root-config --incdir)
CXXFLAGS := -O0 -g -fPIC $(shell root-config --auxcflags)


ROOTFILE ?= nano-CMSSW_11_0_0-RelValZTT-mcRun.root
CONFIG   ?= cmap.json

libCmsNanoAod.so:  CmsNanoAod.hxx CmsNanoAod.cxx CmsNanoAod_dict.cxx
	${CXX} -shared -o $@ ${CPPFLAGS} ${CXXFLAGS} CmsNanoAod.cxx CmsNanoAod_dict.cxx

CmsNanoAod_dict.cxx: CmsNanoAod.hxx
	@rm -f CmsNanoAod_dict.cxx
	rootcling -f CmsNanoAod_dict.cxx CmsNanoAod.hxx CmsNanoAod_LinkDef.h

#===============================================================================

gen-classes: libCmsNanoAod.so
	$(CXX) ${CPPFLAGS} ${CXXFLAGS} -o $@ TestBootstrap.cxx \
		-L. -Wl,-rpath,'$$ORIGIN' \
		-lCmsNanoAod $(shell root-config --libs)

CmsNanoClasses.hxx: gen-classes
	./gen-classes $(ROOTFILE) --fconfig $(CONFIG)


CmsNanoClasses_dict.cxx: CmsNanoClasses.hxx
	@rm -f CmsNanoClasses_dict.cxx
	rootcling -f CmsNanoClasses_dict.cxx CmsNanoClasses.hxx EvdNanoClassesLinkDef.h

EvdGui_dict.cxx: EveMng.hxx EvdGuiLinkDef.h
	@rm -f EvdGui_dict.cxx
	rootcling -f EvdGui_dict.cxx EveMng.hxx proxyBuilders.cxx EvdGuiLinkDef.h

libNanoClassesEvd.so: CmsNanoClasses.hxx CmsNanoClasses_dict.cxx  libCmsNanoAod.so EvdGui_dict.cxx
	$(CXX) -shared -o $@ \
	    $(CPPFLAGS) $(CXXFLAGS) \
	    CmsNanoClasses.cxx CmsNanoClasses_dict.cxx EvdGui_dict.cxx \
	    -L. -lCmsNanoAod

evd: libNanoClassesEvd.so
	$(CXX) ${CPPFLAGS} ${CXXFLAGS} -o $@ evd.cxx EveMng.cxx \
		-L. -Wl,-rpath,'$$ORIGIN' \
		-lNanoClassesEvd -lCmsNanoAod -lEG -lGeom -lROOTWebDisplay -lROOTEve $(shell root-config --libs)

display: evd
	./evd $(ROOTFILE) --fconfig $(CONFIG)

#
#=================================================================================
# print test
TestPrint.o: TestPrint.cxx
	$(CXX) ${CPPFLAGS} ${CXXFLAGS} -c $<	

amtprint: TestPrint.o libNanoClassesEvd.so
	$(CXX) -o $@ TestPrint.o \
		-L. -Wl,-rpath,'$$ORIGIN' \
		-lNanoClassesEvd -lCmsNanoAod $(shell root-config --libs)

#
#===============================================================================
#
.PHONY: evd test test-lib

evdzz: libCmsNanoAod.so
	root.exe -e 'gSystem->Load("libCmsNanoAod.so")' cms_nano_aod_bootstrap.C evd.C

test:
	root.exe CmsNanoAod.cxx+ cms_nano_aod_bootstrap.C cms_nano_aod_test.C

test-lib: libCmsNanoAod.so
	root.exe -e 'gSystem->Load("libCmsNanoAod.so")' cms_nano_aod_bootstrap.C cms_nano_aod_test.C

#===============================================================================

clean:
	rm -rf libCmsNanoAod.so CmsNanoAod_dict*
	rm -rf CmsNanoAod_cxx.d CmsNanoAod_cxx_ACLiC_dict_rdict.pcm CmsNanoAod_cxx.so
	rm -rf CmsNanoClasses.md5 CmsNanoClasses.hxx CmsNanoClasses.cxx
	rm -f TestBootstrap.o
	rm -f gen-classes
	rm -f CmsNanoClasses_dict.cxx
	rm -f EvdGui_dict.cxx
	rm -f libNanoClassesEvd.so
	rm -f evd

cleanevd:
	rm -f CmsNanoClasses.md5 CmsNanoClasses.hxx CmsNanoClasses.cxx
	rm -f nanoClassGen
	rm -f libNanoClassesEvd.so
	rm -f evd