#!/usr/bin/env bash
# Assumes this is run in the build/ folder.

mkdir -p output
../scripts/gen_tasks.py ../data ./output/ 480 | time -p ./asset_conv -n 1
