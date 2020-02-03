evd:    download
	root.exe CmsNanoAod.cxx+ cms_nano_aod_bootstrap.C evd.C

download: nano-CMSSW_11_0_0-RelValZTT-mcRun.root

nano-CMSSW_11_0_0-RelValZTT-mcRun.root:
	wget http://amraktad.web.cern.ch/amraktad/nano-CMSSW_11_0_0-RelValZTT-mcRun.root

test:
	root.exe CmsNanoAod.cxx+ cms_nano_aod_bootstrap.C cms_nano_aod_test.C

clean:
	rm -rf CmsNanoAod_cxx.d CmsNanoAod_cxx_ACLiC_dict_rdict.pcm CmsNanoAod_cxx.so
	rm -rf nano_mama.md5 nano_mama.C
