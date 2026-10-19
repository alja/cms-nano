# Pythia 8 and FastJet are found as described in externals.mk;
# 'make externals' downloads and builds them if needed.
include externals.mk

CXXFLAGS := -O2 -std=c++17 -Wall -fPIC

ifeq ($(shell uname -s),Darwin)
  SONAME    = -Wl,-install_name,@rpath/$@
  EXE_RPATH = -Wl,-rpath,@loader_path
else
  SONAME    =
  EXE_RPATH = -Wl,-rpath,'$$ORIGIN'
endif

all: jetgen libSynthJetGen.so

SynthJetGen.o: SynthJetGen.cc SynthJetGen.h | check-externals
	$(CXX) $(CXXFLAGS) $(PYTHIA_CXXFLAGS) $(FASTJET_CXXFLAGS) \
	    -DPYTHIA8_XMLDOC='"$(PYTHIA_XMLDOC)"' -c $< -o $@

libSynthJetGen.so: SynthJetGen.o | check-externals
	$(CXX) -shared $(SONAME) -o $@ $< $(GEN_LIBS)

jetgen: jetgen.cc SynthJetGen.h libSynthJetGen.so | check-externals
	$(CXX) $(CXXFLAGS) $(PYTHIA_CXXFLAGS) $< -o $@ -L. -lSynthJetGen $(EXE_RPATH) $(GEN_LIBS)

clean:
	rm -f jetgen libSynthJetGen.so SynthJetGen.o
