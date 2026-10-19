# source ../external/env.sh first
PYTHIA_CXXFLAGS  := $(shell pythia8-config --cxxflags | sed 's/-std=c++11//')
FASTJET_CXXFLAGS := $(shell fastjet-config --cxxflags)
CXXFLAGS := -O2 -std=c++17 -Wall -fPIC
GEN_LIBS := $(shell pythia8-config --libs) $(shell fastjet-config --libs)

ifeq ($(shell uname -s),Darwin)
  SONAME    = -Wl,-install_name,@rpath/$@
  EXE_RPATH = -Wl,-rpath,@loader_path
else
  SONAME    =
  EXE_RPATH = -Wl,-rpath,'$$ORIGIN'
endif

all: jetgen libSynthJetGen.so

SynthJetGen.o: SynthJetGen.cc SynthJetGen.h
	$(CXX) $(CXXFLAGS) $(PYTHIA_CXXFLAGS) $(FASTJET_CXXFLAGS) \
	    -DPYTHIA8_XMLDOC='"$(shell pythia8-config --xmldoc)"' -c $< -o $@

libSynthJetGen.so: SynthJetGen.o
	$(CXX) -shared $(SONAME) -o $@ $< $(GEN_LIBS)

jetgen: jetgen.cc SynthJetGen.h libSynthJetGen.so
	$(CXX) $(CXXFLAGS) $(PYTHIA_CXXFLAGS) $< -o $@ -L. -lSynthJetGen $(EXE_RPATH) $(GEN_LIBS)

clean:
	rm -f jetgen libSynthJetGen.so SynthJetGen.o
