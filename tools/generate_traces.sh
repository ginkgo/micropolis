#!/bin/bash


python tools/visualize_trace.py traces/teapot_bounded.trace0 ~/Documents/thesis/figures/trace_teapot.pdf -s0.25
python tools/visualize_trace.py traces/hair_bounded.trace0 ~/Documents/thesis/figures/trace_hair.pdf -s2
python tools/visualize_trace.py traces/columns_bounded.trace0 ~/Documents/thesis/figures/trace_columns.pdf -s2
python tools/visualize_trace.py traces/zinkia_bounded.trace0 ~/Documents/thesis/figures/trace_zinkia.pdf -s20

python tools/visualize_trace.py traces/teapot_local.trace0 ~/Documents/thesis/figures/trace_teapot_local.pdf -s0.25
python tools/visualize_trace.py traces/hair_local.trace0 ~/Documents/thesis/figures/trace_hair_local.pdf -s2
python tools/visualize_trace.py traces/columns_local.trace0 ~/Documents/thesis/figures/trace_columns_local.pdf -s2
python tools/visualize_trace.py traces/zinkia_local.trace0 ~/Documents/thesis/figures/trace_zinkia_local.pdf -s20
