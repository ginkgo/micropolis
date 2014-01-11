#include "utility.h"

kernel void reduce(int batch_size,
            global int* ibuf,
            global int* obuf,
            global int* rbuf)
{
    local int tbuf[128];

    int lid = get_local_id(0);
    int wid = get_group_id(0);
    int gid1 = wid*128 + lid;
    int gid2 = wid*128 + 64 + lid;    

    tbuf[lid] = 0;
    tbuf[lid+64] = 0;

    // read input data into shared buffer
    if (gid1 < batch_size) tbuf[lid]    = ibuf[gid1];
    if (gid2 < batch_size) tbuf[lid+64] = ibuf[gid2];

    // up-sweep
    for (int i = 1; i <= 64; i <<= 1) {
        barrier(CLK_LOCAL_MEM_FENCE);
        
        if (lid < 64/i) {
            int ai = (2*lid+1)*i-1;
            int bi = (2*lid+2)*i-1;
            tbuf[bi] += tbuf[ai];
        }
    }

    // down-sweep
    for (int i = 64; i >= 2; i >>= 1) {
    barrier(CLK_LOCAL_MEM_FENCE);
                                                \
        if (lid < 128/i-1) {
            int ai = i*(lid+1)-1;
            int bi = ai + i/2;

            tbuf[bi] += tbuf[ai];
        }
    }

    // write results back
    barrier(CLK_LOCAL_MEM_FENCE);
    if (gid1 < batch_size) obuf[gid1] = tbuf[lid];
    if (gid2 < batch_size) obuf[gid2] = tbuf[lid+64];

    rbuf[wid] = tbuf[127];    
}
            

kernel void accumulate(int batch_size,
                       global const int* reduced,
                       global int* accumulated)
{
    int gid = get_global_id(0) + 128;

    if (gid < batch_size) {
        accumulated[gid] += reduced[get_group_id(0)];
    }
}
                       

