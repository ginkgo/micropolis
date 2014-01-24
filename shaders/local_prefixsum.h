
#ifndef LOCAL_PREFIXSUM_H
#define LOCAL_PREFIXSUM_H

shared uint prefix_pad[WORK_GROUP_SIZE];

uint prefix_sum(uint v)
{
    uint lid = gl_LocalInvocationID.x;

    prefix_pad[lid] = v;

    // up-sweep
    for (uint i = 1; i <= WORK_GROUP_SIZE/2; i <<= 1) {
        memoryBarrierShared();

        if (lid < (WORK_GROUP_SIZE/2)/i) {
            uint ai = (2*lid+1)*i-1;
            uint bi = (2*lid+2)*i-1;
            prefix_pad[bi] += prefix_pad[ai];
        }
    }

    // down-sweep
    for (uint i = WORK_GROUP_SIZE/2; i >= 2; i >>= 1) {
        memoryBarrierShared();

        if (lid < WORK_GROUP_SIZE/i-1) {
            uint ai = i*(lid+1)-1;
            uint bi = ai + i/2;

            prefix_pad[bi] += prefix_pad[ai];
        }
    }
        
    memoryBarrierShared();
    return prefix_pad[lid];
}


#endif
