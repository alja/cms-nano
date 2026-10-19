#!/usr/bin/env python3
"""Copy the first N events of a NanoAOD file (all branches) to a local file.

Collections (nJet + Jet_*) are rewritten as jagged groups so the output keeps
NanoAOD naming. The small Runs / LuminosityBlocks trees are copied whole.
"""
import argparse
import sys
from collections import defaultdict

import awkward as ak
import uproot

from dump_jets import DEFAULT_URL


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", nargs="?", default=DEFAULT_URL)
    ap.add_argument("-n", "--nevents", type=int, default=100)
    ap.add_argument("-o", "--output", default="nano_100.root")
    args = ap.parse_args()

    fin = uproot.open(args.input, timeout=120)
    events = fin["Events"]
    names = [b.name for b in events.branches]
    prefixes = {m.split("_")[0] for m in names if "_" in m}
    counters = {n[1:] for n in names if n.startswith("n") and n[1:] in prefixes}

    stop = min(args.nevents, events.num_entries)
    print("reading %d branches x %d events ..." % (len(names), stop), file=sys.stderr)
    arr = events.arrays(names, entry_stop=stop, how=dict)

    groups = defaultdict(dict)
    flat = {}
    for n in names:
        prefix = n.split("_")[0]
        if prefix in counters and "_" in n:
            groups[prefix][n[len(prefix) + 1:]] = arr[n]
        elif not (n.startswith("n") and n[1:] in counters):
            flat[n] = arr[n]  # counters are rewritten by uproot

    out_branches = dict(flat)
    for prefix, fields in groups.items():
        out_branches[prefix] = ak.zip(fields)

    with uproot.recreate(args.output) as fout:
        fout["Events"] = out_branches
        for t in ("Runs", "LuminosityBlocks"):
            if t in fin:
                tree = fin[t]
                fout[t] = tree.arrays(tree.keys(), how=dict)

    print("wrote %s: %d events" % (args.output, stop), file=sys.stderr)


if __name__ == "__main__":
    main()
