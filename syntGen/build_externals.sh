#!/bin/bash
# Download, verify and build Pythia 8 and FastJet for libSynthJetGen.
#
#   build_externals.sh <dir>
#
# Sources go to <dir>/src, installs to <dir>/install/{pythia8,fastjet}, build
# logs to <dir>/build-*.log. A package that is already installed is skipped.
# Run through 'make externals' in cms-nano or syntGen.

set -euo pipefail

DEST=${1:?usage: build_externals.sh <dir>}

PYTHIA_VERSION=8318
PYTHIA_URL=https://pythia.org/releases/pythia83/pythia${PYTHIA_VERSION}.tgz
PYTHIA_SHA256=85dce1e623f91499b2973e3f939bf760b0f745ad4f4eb1bd0fbce2074e2e8f5c

FASTJET_VERSION=3.5.2
FASTJET_URL=https://fastjet.fr/repo/fastjet-${FASTJET_VERSION}.tar.gz
FASTJET_SHA256=0b8884a394186a39bd27cef0645a65dd0398d695ae20d29e0559294ee2a44de7

mkdir -p "$DEST/src" "$DEST/install"
DEST=$(cd "$DEST" && pwd)

if [ "$(uname -s)" = Darwin ]; then
   JOBS=$(sysctl -n hw.ncpu)
else
   JOBS=$(nproc)
fi

sha256() {
   if command -v sha256sum > /dev/null; then sha256sum "$1"; else shasum -a 256 "$1"; fi | cut -d' ' -f1
}

# fetch <url> <sha256>: download into $DEST/src unless present, then verify.
fetch() {
   local file="$DEST/src/$(basename "$1")"
   if [ ! -f "$file" ]; then
      echo "Downloading $1"
      curl -fsSL --retry 3 -o "$file.part" "$1"
      mv "$file.part" "$file"
   fi
   if [ "$(sha256 "$file")" != "$2" ]; then
      echo "ERROR: checksum mismatch for $file; delete it and run again." >&2
      exit 1
   fi
}

# macOS: a Command Line Tools linker older than the newest SDK cannot read
# that SDK's .tbd files, so nothing links. Then use the newest SDK that works.
pick_macos_sdk() {
   [ "$(uname -s)" = Darwin ] || return 0
   local test=$DEST/src/linktest
   echo 'int main() { return 0; }' > "$test.cc"
   if c++ "$test.cc" -o "$test" 2> /dev/null; then
      rm -f "$test" "$test.cc"
      return 0
   fi
   local sdk
   for sdk in $(ls -d /Library/Developer/CommandLineTools/SDKs/MacOSX[0-9]*.sdk 2> /dev/null | sort -rV); do
      if SDKROOT=$sdk c++ "$test.cc" -o "$test" 2> /dev/null; then
         export SDKROOT=$sdk
         echo "NOTE: the default macOS SDK does not link; using SDKROOT=$sdk."
         echo "      Set it in your environment for the rest of the build too."
         rm -f "$test" "$test.cc"
         return 0
      fi
   done
   rm -f "$test" "$test.cc"
   echo "ERROR: cannot link a test program with any macOS SDK." >&2
   exit 1
}

# build <name> <tarball> <srcdir> <prefix> <configure args...>
build() {
   local name=$1 tarball=$2 srcdir=$3 prefix=$4
   shift 4
   if [ -d "$prefix/bin" ]; then
      echo "$name already installed in $prefix"
      return 0
   fi
   local log=$DEST/build-$name.log
   echo "Building $name (log: $log)"
   rm -rf "$DEST/src/$srcdir"
   tar -xzf "$DEST/src/$tarball" -C "$DEST/src"
   (
      cd "$DEST/src/$srcdir"
      ./configure --prefix="$prefix" "$@"
      make -j"$JOBS"
      make install
   ) > "$log" 2>&1 || { echo "ERROR: $name build failed, see $log" >&2; exit 1; }
}

fetch "$PYTHIA_URL" "$PYTHIA_SHA256"
fetch "$FASTJET_URL" "$FASTJET_SHA256"
pick_macos_sdk

build fastjet "fastjet-${FASTJET_VERSION}.tar.gz" "fastjet-${FASTJET_VERSION}" \
   "$DEST/install/fastjet" --enable-allcxxplugins
build pythia8 "pythia${PYTHIA_VERSION}.tgz" "pythia${PYTHIA_VERSION}" \
   "$DEST/install/pythia8"

echo "Pythia 8 and FastJet are installed in $DEST/install"
