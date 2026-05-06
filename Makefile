CPPFLAGS := -I$(shell root-config --incdir)
CXXFLAGS := -O0 -g -fPIC $(shell root-config --auxcflags)

libCmsNanoAod.so:  CmsNanoAod.hxx CmsNanoAod.cxx CmsNanoAod_dict.cxx
	${CXX} -shared -o $@ ${CPPFLAGS} ${CXXFLAGS} CmsNanoAod.cxx CmsNanoAod_dict.cxx

CmsNanoAod_dict.cxx: CmsNanoAod.hxx CmsNanoClasses.hxx
	@rm -f CmsNanoAod_dict.cxx
	rootcling CmsNanoAod_dict.cxx CmsNanoAod.hxx CmsNanoClasses.hxx

#===============================================================================

.PHONY: evd

CmsNanoClasses.hxx:
	root.exe -q CmsNanoAod.cxx cms_nano_aod_bootstrap.C

evd: libCmsNanoAod.so
	root.exe -e 'gSystem->Load("libCmsNanoAod.so")' cms_nano_aod_bootstrap.C evd.C

test:
	root.exe CmsNanoAod.cxx+ cms_nano_aod_bootstrap.C cms_nano_aod_test.C

test-lib: libCmsNanoAod.so
	root.exe -e 'gSystem->Load("libCmsNanoAod.so")' cms_nano_aod_bootstrap.C cms_nano_aod_test.C

clean:
	rm -rf libCmsNanoAod.so CmsNanoAod_dict*
	rm -rf CmsNanoAod_cxx.d CmsNanoAod_cxx_ACLiC_dict_rdict.pcm CmsNanoAod_cxx.so
	rm -rf CmsNanoClasses.md5 CmsNanoClasses.hxx
