# ===== Four-lepton ntuple workflow (RunIII2024 NanoAODv15) =====
# Build with the CMSSW ROOT set up via ~/rooutil/bin/setuproot.sh (see setup.sh):
#
#   source setup.sh      # or: source ~/rooutil/bin/setuproot.sh
#   make
#
# The rdfutil toolkit header is pulled from the local clone in code/rdfutil,
# so there is a single source of truth for it (no vendored copy here).

# ===== User configuration =====
CXX        := g++
RDFUTIL    := code/rdfutil
CXXFLAGS   := -O2 -Wall -Wextra -std=c++17 -I$(RDFUTIL)
ROOTCFLAGS := $(shell root-config --cflags)
ROOTLIBS   := $(shell root-config --libs)

# ===== Sources and targets =====
SRCS := $(wildcard *.cc)
BINS := $(SRCS:.cc=)

# ===== Build rules =====
all: $(BINS)

%: %.cc $(RDFUTIL)/rdfutil.h
	$(CXX) $(CXXFLAGS) $(ROOTCFLAGS) -o $@ $< $(ROOTLIBS)

clean:
	rm -f $(BINS) *.o

rebuild: clean all

.PHONY: all clean rebuild
