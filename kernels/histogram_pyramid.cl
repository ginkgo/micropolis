
kernel void reduce (int count, int offset1, int offset2, global int* buffer)
{
    int gid = get_global_id(0);

    if (gid*2+0 < count) {
        buffer[gid + offset2] = buffer[gid*2 + 0 + offset1];
    }

    if (gid*2+1 < count) {
        buffer[gid + offset2] += buffer[gid*2 + 1 + offset1];
    }
}
