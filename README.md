# cms-nano

Web-based event display for CMS NanoAOD files, built on ROOT's EVE 7 (REve).

![screenshot](https://user-images.githubusercontent.com/2516492/78937920-cebbed80-7aa0-11ea-9ab9-7c8b5cd14957.png)

## Requirements

- ROOT built from the `master` branch, with `root7`, `webgui` and `http` enabled (see below)
- A C++17 compiler (GCC or Clang)
- [nlohmann/json](https://github.com/nlohmann/json) headers (Fedora: `json-devel`, Debian/Ubuntu: `nlohmann-json3-dev`)
- `jq`, which the Makefile uses to generate the proxy-builder LinkDef from the config
- A web browser to view the display

## Building ROOT from master

EVE 7 changes often, so use a recent ROOT built from `master` rather than a release.

```bash
mkdir -p ~/root-dev/master && cd ~/root-dev/master
git clone --branch master https://github.com/root-project/root.git root

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug \
      -Dbuiltin_openui5=ON \
      -Dbuiltin_mathjax=ON \
      -Dbuiltin_civetweb=ON \
      -Dbuiltin_vdt=ON \
      -Dbuiltin_xrootd=ON \
      ../root
cmake --build . -j$(nproc)
```

`-Dbuiltin_openui5=ON` matters: without it, ROOT configures without the web GUI (it only warns) and EVE 7 can't open a display. `root7`, `webgui` and `http` are on by default. If CMake stops on a missing optional package (for example cfitsio), turn that feature off, e.g. `-Dfitsio=OFF`. Use `-DCMAKE_BUILD_TYPE=RelWithDebInfo` instead of `Debug` if you don't need to debug ROOT itself.

Check the build:

```bash
source ~/root-dev/master/build/bin/thisroot.sh
root-config --version
root-config --features | tr ' ' '\n' | grep -E '^(root7|webgui|http)$'
```

## Building cms-nano

```bash
source ~/root-dev/master/build/bin/thisroot.sh   # in every new shell

git clone https://github.com/alja/cms-nano.git
cd cms-nano
make evd
```

The build has three steps:

1. `libCmsNanoAod.so`: generic NanoAOD reading code.
2. `gen-classes` reads the data file and the config, and generates `CmsNanoClasses.hxx/.cxx`, one class per collection.
3. `libNanoClassesEvd.so` and `evd`: the generated classes, the proxy builders and the event display executable.

The generated classes depend on the data file's branches. To switch to a file with a different layout, rebuild from scratch:

```bash
make clean
make evd ROOTFILE=/path/to/other_nano.root
```

After upgrading ROOT, also run `make clean` before rebuilding.

## Running

```bash
make display
```

This builds what is out of date and runs:

```bash
./evd BTag_100.root --fconfig cmap.json
```

`evd` prints a URL, `http://localhost:9092/win1/`. Open it in a browser. If port 9092 is in use (for example by another `evd`), ROOT picks a different port and prints that one instead.

To use a different data file or config:

```bash
make display ROOTFILE=/path/to/nano.root CONFIG=my_config.json
```

### Running on a remote machine

Forward the port over SSH, then open the printed URL locally:

```bash
ssh -L 9092:localhost:9092 user@remote-host
```

## Data

`BTag_100.root` holds the first 100 events of a BTag NanoAOD sample and is included in the repository so the display works right after cloning. It is the default `ROOTFILE`.

## Configuration

`cmap.json` lists the collections to load and how to draw them:

```json
{
  "collections": [
    { "name": "EventInfo" },
    { "name": "Jet", "proxyBuilder": "JetProxyBuilder<nanoaod::Jet>", "color": 632 },
    { "name": "HLT" }
  ]
}
```

- `name` is the NanoAOD collection (branch prefix) name.
- `proxyBuilder` is optional: a class from `proxyBuilders.cxx` that draws the collection. Collections without one are loaded and shown in tables, but not drawn in 3D.
- `color` is a ROOT color index (632 is `kRed`).

Table columns are set in `EventManager::createScenesAndViews()` in `EveMng.cxx`.

## Make targets

| Target    | What it does |
|-----------|--------------|
| `evd`     | Build the event display |
| `display` | Build and run the event display |
| `amtprint`| Build a test program that prints collection contents |
| `clean`   | Remove all build outputs and generated classes |
