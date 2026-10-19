#!/usr/bin/env python3
"""Dump NanoAOD jets to a plain-text table that jetgen reads.

One line per jet. Only the branches jetgen needs are read, so remote files
over HTTPS are cheap (a few MB for a handful of events).
"""
import argparse
import sys

import awkward as ak
import uproot

DEFAULT_URL = (
    "https://opendata.cern.ch/eos/opendata/cms/mc/RunIISummer20UL16NanoAODv9/"
    "TTToSemiLeptonic_TuneCP5_13TeV-powheg-pythia8/NANOAODSIM/"
    "106X_mcRun2_asymptotic_v17-v1/120000/08FCB2ED-176B-064B-85AB-37B898773B98.root"
)

EVENT_BRANCHES = ["run", "luminosityBlock", "event", "PV_x", "PV_y", "PV_z"]
JET_BRANCHES = [
    "pt", "eta", "phi", "mass", "rawFactor", "nConstituents",
    "chHEF", "neHEF", "chEmEF", "neEmEF", "muEF", "qgl", "jetId",
]
# MC-truth only; filled with 0 for data.
JET_MC_BRANCHES = ["partonFlavour", "hadronFlavour"]

COLUMNS = (["iev", "run", "lumi", "event", "ijet"] + JET_BRANCHES
           + JET_MC_BRANCHES + ["pvx", "pvy", "pvz"])


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", nargs="?", default=DEFAULT_URL,
                    help="NanoAOD file (local path or https URL)")
    ap.add_argument("-n", "--nevents", type=int, default=5)
    ap.add_argument("-s", "--start", type=int, default=0, help="first entry")
    ap.add_argument("--ptmin", type=float, default=20.0)
    ap.add_argument("-o", "--output", default="jets.txt")
    args = ap.parse_args()

    tree = uproot.open(args.input, timeout=60)["Events"]
    keys = set(tree.keys())
    mc = [b for b in JET_MC_BRANCHES if "Jet_" + b in keys]
    branches = EVENT_BRANCHES + ["Jet_" + b for b in JET_BRANCHES + mc]
    stop = min(args.start + args.nevents, tree.num_entries)
    arr = tree.arrays(branches, entry_start=args.start, entry_stop=stop)

    nout = 0
    with open(args.output, "w") as out:
        out.write("# source: %s\n" % args.input)
        out.write("# " + " ".join(COLUMNS) + "\n")
        for i in range(len(arr)):
            ev = arr[i]
            for j in range(len(ev["Jet_pt"])):
                if ev["Jet_pt"][j] < args.ptmin:
                    continue
                row = [args.start + i, ev["run"], ev["luminosityBlock"], ev["event"], j]
                row += [ev["Jet_" + b][j] for b in JET_BRANCHES]
                row += [ev["Jet_" + b][j] if b in mc else 0 for b in JET_MC_BRANCHES]
                row += [ev["PV_x"], ev["PV_y"], ev["PV_z"]]
                out.write(" ".join(fmt(v) for v in row) + "\n")
                nout += 1

    print("wrote %d jets from %d events to %s" % (nout, len(arr), args.output),
          file=sys.stderr)


def fmt(v):
    v = v.item() if hasattr(v, "item") else v
    return "%.6g" % v if isinstance(v, float) else str(v)


if __name__ == "__main__":
    main()
