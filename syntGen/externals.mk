# Pythia 8 and FastJet for libSynthJetGen, used in that order of preference:
#   1. install prefixes given as PYTHIA8=... / FASTJET=...
#   2. pythia8-config / fastjet-config in PATH (e.g. an LCG view on cvmfs)
#   3. the copies 'make externals' builds in $(EXTERNALS)
# PYTHIA8_CONFIG / FASTJET_CONFIG can also be set directly.

SYNTHGEN_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
EXTERNALS    ?= $(abspath $(SYNTHGEN_DIR)/../external)

# find_config <tool> <prefix variable> <install subdir>
find_config = $(firstword $(if $(2),$(wildcard $(2)/bin/$(1))) \
                          $(shell command -v $(1) 2> /dev/null) \
                          $(wildcard $(EXTERNALS)/install/$(3)/bin/$(1)))

PYTHIA8_CONFIG ?= $(call find_config,pythia8-config,$(PYTHIA8),pythia8)
FASTJET_CONFIG ?= $(call find_config,fastjet-config,$(FASTJET),fastjet)

ifneq ($(and $(PYTHIA8_CONFIG),$(FASTJET_CONFIG)),)
  PYTHIA_CXXFLAGS  := $(shell $(PYTHIA8_CONFIG) --cxxflags | sed 's/-std=c++11//')
  PYTHIA_XMLDOC    := $(shell $(PYTHIA8_CONFIG) --xmldoc)
  FASTJET_CXXFLAGS := $(shell $(FASTJET_CONFIG) --cxxflags)
  GEN_LIBS         := $(shell $(PYTHIA8_CONFIG) --libs) $(shell $(FASTJET_CONFIG) --libs)
endif

.PHONY: check-externals externals

# Order-only prerequisite of everything that needs Pythia 8 / FastJet.
check-externals:
	@if [ -z "$(PYTHIA8_CONFIG)" ] || [ -z "$(FASTJET_CONFIG)" ]; then \
	   echo "Pythia 8 and FastJet are needed for the synthetic jet tracks; not found:"; \
	   [ -n "$(PYTHIA8_CONFIG)" ] || echo "   pythia8-config"; \
	   [ -n "$(FASTJET_CONFIG)" ] || echo "   fastjet-config"; \
	   echo "Either"; \
	   echo "   - put pythia8-config and fastjet-config in PATH (e.g. source an LCG view),"; \
	   echo "   - pass the install prefixes: make PYTHIA8=<dir> FASTJET=<dir>,"; \
	   echo "   - run 'make externals' to download and build them into $(EXTERNALS), or"; \
	   echo "   - build cms-nano without synthetic tracks: make SYNTH=0"; \
	   exit 1; \
	fi

externals:
	bash $(SYNTHGEN_DIR)/build_externals.sh $(EXTERNALS)
