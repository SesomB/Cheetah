#!/bin/bash -e

sudo perf record --call-graph dwarf --aio --sample-cpu ./build/bin/cheetah