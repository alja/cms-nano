# syntGen — synthetic jet constituents for NanoAOD jets

Pythia 8 standalone shower + hadronization per NanoAOD jet, matched in
pT / eta / phi.

- `SynthJetGen.h/.cc` -> `libSynthJetGen.so`: the generator. cms-nano's
  JetProxyBuilder uses it (see `../SynthTracks.h`); the header has no Pythia /
  FastJet includes, so rootcling can parse it.
- `jetgen`: runs the generator on a jet table and writes the particles.
- `dump_jets.py`, `skim_nano.py`, `make_display.py`: NanoAOD jet dumping,
  NanoAOD skimming and a small standalone HTML event display (Python, uproot).

`make` needs `pythia8-config` and `fastjet-config` in `PATH`. On the
development Mac they come from `~/cms-dev/external/env.sh`, which also has the
uproot venv for the Python scripts:

```bash
source ~/cms-dev/external/env.sh       # Pythia 8.318, FastJet 3.5.2, uproot venv
make
python dump_jets.py -n 5 -o jets.txt   # default: CMS Open Data ttbar NanoAODv9 over HTTPS
./jetgen jets.txt particles.txt [seed]
```

- `jets.txt`: one line per jet (pt > 20 GeV), raw factor, constituent counts,
  energy fractions, qgl, MC flavour, primary vertex.
- `particles.txt`: `iev event ijet pdgId charge px py pz e` for the generated
  constituents of each jet.
- stdout: target (raw) vs generated jet per row, plus generation time.

Notes
- `env.sh` pins the macOS 26.5 SDK: the Command Line Tools linker cannot
  read the MacOSX27.0.sdk `.tbd` files.
- Use `https://opendata.cern.ch/eos/opendata/...` URLs; `eospublic.cern.ch`
  over HTTPS uses a CERN CA that Python does not trust.
- NanoAODv9 has no `Jet_chMultiplicity`; only `Jet_nConstituents`.

Local sample
- `~/cms-dev/data/TTToSemiLeptonic_UL16NanoAODv9_100ev.root`: first 100 events of the
  Open Data ttbar file, all 1504 Events branches plus Runs / LuminosityBlocks,
  made with `python skim_nano.py -n 100 -o <out.root>`.
- Values are identical to the original; the only difference is that the
  `nX` counter branches are `Int_t` instead of `UInt_t` (uproot writes them
  that way).

Event display
- `python make_display.py` reads `jets.txt` + `particles.txt` and writes a
  self-contained `event_display.html` (open it directly in a browser).
- Transverse and side views; charged particles are stepped as helices in a
  uniform 3.8 T field in the page (no energy loss / multiple scattering),
  neutrals are straight dashed lines to the ECAL. Hover a track or a table row
  to highlight its jet.
