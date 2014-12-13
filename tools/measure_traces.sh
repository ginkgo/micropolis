#!/bin/bash


for n in "teapot" "hair" "columns" "zinkia" "zinkia1" "zinkia2" "zinkia3" "eye_split"
#for n in "teapot" "hair" "columns" "zinkia"
do
    echo -n $n...
    ./micropolis --trace_file=traces/${n}_bounded.trace --bound_n_split_method=BOUNDED --input_file=mscene/$n.mscene --dump_mode=true --windowless=true >> /dev/null
    ./micropolis --trace_file=traces/${n}_local.trace --bound_n_split_method=LOCAL --input_file=mscene/$n.mscene --dump_mode=true --windowless=true >> /dev/null
    echo " done"
done

#S=zinkia && python tools/visualize_trace.py $S.trace ~/Documents/thesis/figures/trace_$S.pdf -s20
