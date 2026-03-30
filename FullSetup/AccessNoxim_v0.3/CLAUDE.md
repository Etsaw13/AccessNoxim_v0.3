# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and run

- Main simulator build:
  - `cd bin && make`
- Clean simulator artifacts:
  - `cd bin && make clean`
- Run simulator (from `bin/` after build):
  - `./noxim [args]`

## Environment requirements

- This project builds against SystemC via `bin/Makefile.defs`.
- Set `SYSTEMC` in `bin/Makefile.defs` to your local SystemC install path.
- Default target architecture in `bin/Makefile` is `TARGET_ARCH = linux64`.

## Optional tooling (under `other/`)

- Build optional helper tools:
  - `cd other && make`
- Utilities described in `other/README.txt`:
  - `apsra2noxim`: extracts communication/routing tables from APSRA output
  - `noxim_explorer`: design-space exploration and MATLAB-format result export
  - `mapping2cg`: converts communication trace to mapped communication trace

## High-level architecture

- Entry point is `src/NoximMain.cpp` (`sc_main`):
  - Parses CLI options (`parseCmdLine`)
  - Instantiates `NoximNoC` SystemC top module
  - Drives reset + simulation (`sc_start`)
  - Produces logs via `NoximLog`
  - Prints final metrics via `NoximGlobalStats`

- Core NoC simulation pipeline:
  - `NoximNoC` wires the mesh/tiles and simulation signals
  - `NoximTile` composes router + processing element
  - `NoximRouter` handles routing/arb, buffering, reservations, and forwarding
  - `NoximProcessingElement` injects/ejects traffic according to configured distribution

- Routing, traffic, and observability support:
  - Routing data: `NoximGlobalRoutingTable`, `NoximLocalRoutingTable`
  - Traffic data: `NoximGlobalTrafficTable`
  - Metrics: `NoximStats`, `NoximGlobalStats`
  - Logging/traces: `NoximLog`

- Thermal/power co-simulation:
  - C++ integration layer: `thermal_IF.*`
  - Thermal model implementation is mostly C files (`temperature*`, `flp`, `package`, `shape`, `RCutil`, `npe`, `util`)
  - Power/thermal logs are generated during simulation and written under `bin/results` (and related output directories created at runtime)

## Notes for code changes

- Build inputs are explicitly listed in `bin/Makefile` (`SRCS` and `SRCS_C`); if you add new translation units, update those lists.
- Command-line behavior is centralized around `NoximCmdLineParser` and global runtime config in `NoximGlobalParams` (initialized in `NoximMain.cpp`).
- There is no dedicated lint or unit-test framework configured in this repository; validation is done by rebuilding and running the simulator with representative configs.
