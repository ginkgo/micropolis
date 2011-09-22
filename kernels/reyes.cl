
// Compile time constants:
// PATCH_SIZE            - int
// TILE_SIZE             - int
// GRID_SIZE             - int2
// VIEWPORT_MIN_PIXEL    - int2
// VIEWPORT_MAX_PIXEL    - int2
// VIEWPORT_SIZE_PIXEL   - int2
// MAX_BLOCK_COUNT       - int
// MAX_BLOCK_ASSIGNMENTS - int

#define BLOCKS_PER_LINE (PATCH_SIZE/8)
#define BLOCKS_PER_PATCH (BLOCKS_PER_LINE*BLOCKS_PER_LINE)

#define PXLCOORD_SHIFT 10

#define VIEWPORT_MIN  (VIEWPORT_MIN_PIXEL  << PXLCOORD_SHIFT)
#define VIEWPORT_MAX  (VIEWPORT_MAX_PIXEL  << PXLCOORD_SHIFT)
#define VIEWPORT_SIZE (VIEWPORT_SIZE_PIXEL << PXLCOORD_SHIFT)

inline int calc_framebuffer_pos(int2 pxlpos)
{
    int2 gridpos = pxlpos / TILE_SIZE;
    int  grid_id = gridpos.x + GRID_SIZE.x * gridpos.y;
    int2 loclpos = pxlpos - gridpos * TILE_SIZE;
    int  locl_id = loclpos.x + TILE_SIZE * loclpos.y;

    return grid_id * TILE_SIZE * TILE_SIZE + locl_id;
}

float4 eval_spline(float4 p0, float4 p1, float4 p2, float4 p3, float t)
{
    float s = 1-t;
    return s*s*s*p0 + 3*s*s*t*p1 + 3*s*t*t*p2 + t*t*t*p3;
}

float4 eval_patch(private float4* patch, float2 st)
{
    float4 P[4];

    for (int i = 0; i < 4; ++i) {
        P[i] = eval_spline(patch[4 * i + 0],
                           patch[4 * i + 1],
                           patch[4 * i + 2],
                           patch[4 * i + 3], st.y);
    }

    return eval_spline(P[0],P[1],P[2],P[3], st.x);
}

float4 mul_m44v4(float16 mat, float4 vec)
{
    return (float4) {dot(mat.s048C, vec),
                     dot(mat.s159D, vec),
                     dot(mat.s26AE, vec),
                     dot(mat.s37BF, vec)};

}

inline size_t calc_grid_pos(size_t nu, size_t nv, size_t patch)
{
    return nu + nv * (PATCH_SIZE+1) + patch * (PATCH_SIZE+1)*(PATCH_SIZE+1);
}

__kernel void dice (const global float4* patch_buffer,
                    global float4* pos_grid,
                    global int2* pxlpos_grid,
                    float16 proj,
                    global float* depth_grid)
{
    float4 patch[16];

    if (get_global_id(0) > PATCH_SIZE ||
        get_global_id(1) > PATCH_SIZE) {
        return;
    }

    size_t nv = get_global_id(0), nu = get_global_id(1);
    size_t patch_id = get_global_id(2);

    for (int i = 0; i < 16; ++i) {
        patch[i] = patch_buffer[patch_id * 16 + i];
    }

    float2 uv = (float2) {nu/(float)PATCH_SIZE, nv/(float)PATCH_SIZE};

    float4 pos = eval_patch(patch, uv);

    pos.y += native_sin(pos.x*15) * 0.05f;
    
    float4 p = mul_m44v4(proj, pos);

    int2 coord = {(int)(p.x/p.w * VIEWPORT_SIZE.x/2 + VIEWPORT_SIZE.x/2),
                  (int)(p.y/p.w * VIEWPORT_SIZE.y/2 + VIEWPORT_SIZE.y/2)};


    int grid_index = calc_grid_pos(nu, nv, patch_id);
    pos_grid[grid_index] = pos;
    pxlpos_grid[grid_index] = coord;
    depth_grid[grid_index] = p.z/p.w;
}


inline int is_front_facing(const int2 *ps)
{
    if (BACKFACE_CULLING) {
        int2 d1 = ps[1] - ps[0];
        int2 d2 = ps[2] - ps[0];
        int2 d3 = ps[3] - ps[0];

        return (d1.x*d3.y-d3.x*d1.y < 0 || d3.x*d2.y-d2.x*d3.y < 0);
    } else {
        return 1;
    }
}

inline int is_empty(int2 min, int2 max)
{
    return min.x >= max.x && min.y >= max.y;
}

inline int calc_block_pos(int u, int v, int patch_id)
{
    return u + v * BLOCKS_PER_LINE + patch_id * BLOCKS_PER_PATCH;
}

inline int calc_color_grid_pos(int u, int v, int patch_id)
{
    return u + v * PATCH_SIZE + patch_id * (PATCH_SIZE*PATCH_SIZE);
}

__kernel void shade(const global float4* pos_grid,
                    const global int2* pxlpos_grid,
                    global int4* block_index,
                    global float4* color_grid)
{
    volatile local int x_min;
    volatile local int y_min;
    volatile local int x_max;
    volatile local int y_max;

    if (get_local_id(0) == 0 && get_local_id(1) == 0) {
        x_min = VIEWPORT_MAX.x;
        y_min = VIEWPORT_MAX.y;
        x_max = VIEWPORT_MIN.x;
        y_max = VIEWPORT_MIN.y;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    // V
    // |
    // 2 - 3
    // | / |
    // 0 - 1 - U

    float4 pos[4];
    int2 pxlpos[4];

    int nv = get_global_id(0), nu = get_global_id(1);
    int patch_id = get_global_id(2);

    int2 pmin = VIEWPORT_MAX;
    int2 pmax = VIEWPORT_MIN;

    for     (int vi = 0; vi < 2; ++vi) {
        for (int ui = 0; ui < 2; ++ui) {
            int i = ui + vi * 2;
            pos[i] = pos_grid[calc_grid_pos(nu+ui, nv+vi, patch_id)];
            int2 p  = pxlpos_grid[calc_grid_pos(nu+ui, nv+vi, patch_id)];

            pmin = min(pmin, p);
            pmax = max(pmax, p);

            pxlpos[i] = p;
        }
    }

    if (is_front_facing(pxlpos)) {
        atomic_min(&x_min, pmin.x);
        atomic_min(&y_min, pmin.y);
        atomic_max(&x_max, pmax.x);
        atomic_max(&y_max, pmax.y);

        float3 du = (pos[1] - pos[0] +
                     pos[3] - pos[2]).xyz * 0.5f;
        float3 dv = (pos[2] - pos[0] +
                     pos[3] - pos[1]).xyz * 0.5f;

        float3 n = normalize(cross(dv,du));

        float3 l = normalize((float3){4,3,8});

        float3 v = -normalize((pos[0]+pos[1]+pos[2]+pos[3]).xyz);

        float4 dc = {0.1f, 0.2f, 1, 1};
        float4 sc = {1, 1, 1, 1};
        
        float3 h = normalize(l+v);
        
        float sh = 60.0f;

        float4 c = max(dot(n,l),0) * dc + pow(max(dot(n,h), 0), sh) * sc;

        color_grid[calc_color_grid_pos(nu, nv, patch_id)] = c;
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_local_id(0) == 0 &&  get_local_id(1) == 0) {
        x_min = max(VIEWPORT_MIN.x, x_min);
        y_min = max(VIEWPORT_MIN.y, y_min);
        x_max = min(VIEWPORT_MAX.x, x_max);
        y_max = min(VIEWPORT_MAX.y, y_max);

        int i = calc_block_pos(get_group_id(0), get_group_id(1), get_group_id(2));
        block_index[i] = (int4){x_min, y_min, x_max, y_max};
    }
}

__kernel void clear_heads(global int* heads)
{
    heads[get_global_id(0)] = -1;
}

inline int calc_node_pos(int block_id, int assign_cnt)
{
    if (assign_cnt >= MAX_BLOCK_ASSIGNMENTS) {
        return -2;
    }

    return block_id + assign_cnt * MAX_BLOCK_COUNT;
}

inline int calc_tile_id(int tx, int ty)
{
    return tx + FRAMEBUFFER_SIZE.x/8 * ty;
}

__kernel void assign(global const int4* block_index,
                     volatile global int* heads,
                     global int2* node_heap)
{
    int block_id = get_global_id(0);
    int4 block_bound = block_index[block_id];

    int assign_cnt = 0;

    if (is_empty(block_bound.xy, block_bound.zw)) {
        // Empty block
        return;
    }

    int2 min_tile = block_bound.xy >> (PXLCOORD_SHIFT + 3);
    int2 max_tile = block_bound.zw >> (PXLCOORD_SHIFT + 3);

    for     (int ty = min_tile.y; ty <= max_tile.y; ++ty) {
        for (int tx = min_tile.x; tx <= max_tile.x; ++tx) {
            int tile_id = calc_tile_id(tx,ty);
            int heap_pos = calc_node_pos(block_id, assign_cnt);

            ++assign_cnt;

            int old_head = atomic_xchg(heads + tile_id, heap_pos);

            if (heap_pos >= 0) {
                node_heap[heap_pos] = (int2){block_id, old_head};
            }
        }
    }

}

inline void recover_patch_pos(size_t block_id, size_t lx, size_t ly,
                              private size_t* u, private size_t* v, private size_t* patch)
{
    *patch = block_id / BLOCKS_PER_PATCH;
    size_t local_block_id = block_id % BLOCKS_PER_PATCH;
    size_t bv = local_block_id / BLOCKS_PER_LINE;
    size_t bu = local_block_id % BLOCKS_PER_LINE;

    *u = bv * 8 + lx;
    *v = bu * 8 + ly;
}

inline int4 idot4 (int4 Ax, int4 Ay, int4 Bx, int4 By)
{
    return mad24(Ax, Bx, mul24(Ay,  By));
}


inline int idot (int2 a, int2 b)
{
    return mad24(a.x, b.x, mul24(a.y,  b.y));
}

/* int inside_quad2(int4 Px, int4 Py, int2 tp, float4* weights) */
/* { */

/*     //   2z     TT      3w */
/*     //    +<-----------+ */
/*     //    |          ++A */
/*     //    |        ++  | */
/*     //  RR|     MM     |LL */
/*     //    |  ++        | */
/*     //    V++          | */
/*     //    +----------->+ */
/*     //   0x     BB      1y */
/*     // */
/*     // (M goes 0 -> 3) */


/*     *weights = (float4){0.25f,0.25f,0.25f,0.25f}; */

/*     return ; */
/* } */

int inside_quad(const int2* ps, int2 tp, float4* weights)
{
    // TODO: Vectorize


    //   2      TT      3
    //    +<-----------+
    //    |          ++A
    //    |        ++  |
    //  RR|     MM     |LL
    //    |  ++        |
    //    V++          |
    //    +----------->+
    //   0      BB      1
    //
    // (M goes 0 -> 3)


    int2 dm = ps[3] - ps[0];
    int2 db = ps[1] - ps[0];
    int2 dl = ps[3] - ps[1];
    int2 dr = ps[0] - ps[2];
    int2 dt = ps[2] - ps[3];

    dm = (int2){dm.y, -dm.x};
    db = (int2){db.y, -db.x};
    dl = (int2){dl.y, -dl.x};
    dr = (int2){dr.y, -dr.x};
    dt = (int2){dt.y, -dt.x};

    int om = idot(dm, ps[3]);
    int ob = idot(db, ps[1]);
    int ol = idot(dl, ps[3]);    
    int or = idot(dr, ps[0]);
    int ot = idot(dt, ps[2]);    

    int vm = idot(tp, dm) - om;

    if (vm > 0) {
        int vr = idot(tp, dr) - or;
        int vt = idot(tp, dt) - ot;

        *weights = (float4){(float)vt/(idot(ps[0], dt) - ot),
                            0,
                            (float)vm/(idot(ps[2], dm) - om),
                            (float)vr/(idot(ps[3], dr) - or)};                           
                            

        return  vr >= 0 && vt >= 0;
    } else {
        int vl = idot(tp, dl) - ol;
        int vb = idot(tp, db) - ob;

        *weights = (float4){(float)vl/(idot(ps[0], dl) - ol),
                            (float)vm/(idot(ps[1], dm) - om),
                            0,
                            (float)vb/(idot(ps[3], db) - ob)};                           
                            

        return  vl >= 0 && vb >= 0;
    }
}

typedef int lock_t;

#define LOCK(lock)  \
    while (1) {  \
    if (atomic_cmpxchg(&(lock), 1, 0)) continue;

#define UNLOCK(lock) \
    atomic_xchg(&(lock), 1);                    \
    break;                                  \
    }

__kernel void sample(global const int* heads,
                     global const int2* node_heap,
                     global const int2* pxlpos_grid,
                     global float4* framebuffer,
                     global const float4* color_grid,
                     global const float* depth_grid,
                     int first_sample)
{
    local float4 colors[8][8];
    local float depths[8][8];
    local int locks[8][8];
    

    int tile_id  = calc_tile_id(get_group_id(0), get_group_id(1));
    int next = heads[tile_id];

    if (next < 0) return;

    const int2 o = (int2){get_group_id(0), get_group_id(1)} << (3 + PXLCOORD_SHIFT);
    const int2 g = {get_global_id(0), get_global_id(1)};
    const int2 l = {get_local_id(0), get_local_id(1)};

    int fb_id = calc_framebuffer_pos(g);

    locks[l.y][l.x] = 1;

    float4 c = framebuffer[fb_id];
    colors[l.y][l.x] = c;
    depths[l.y][l.x] = c.w;

    barrier(CLK_LOCAL_MEM_FENCE);

    while (next >= 0) {
        int2 node = node_heap[next];
        next = node.y;
        int block_id = node.x;

        // V
        // |
        // 2 - 3
        // | / |
        // 0 - 1 - U
        int Pxa[4], Pya[4];
        float ds[4];

        int2 min_p = (int2) {8<<PXLCOORD_SHIFT, 8<<PXLCOORD_SHIFT};
        int2 max_p = (int2) {0,0};

        size_t patch_id, u, v;
        recover_patch_pos(block_id, l.x, l.y,  &u, &v, &patch_id);

        for (size_t idx = 0; idx < 4; ++idx) {
            size_t p = calc_grid_pos(u+(idx&1), v+(idx>>1), patch_id);
            int2 pxlpos = pxlpos_grid[p] - o;
            Pxa[idx] = pxlpos.x;
            Pya[idx] = pxlpos.y;

            min_p = min(min_p, pxlpos);
            max_p = max(max_p, pxlpos);

            float depth = depth_grid[p];
            ds[idx] = depth;
        }

        float4 c = color_grid[calc_color_grid_pos(u, v, patch_id)];

        min_p = max(min_p, (int2) {0,0});
        max_p = min(max_p, (int2) {8<<PXLCOORD_SHIFT, 8<<PXLCOORD_SHIFT});

        /* if (is_empty(min_p, max_p) || !is_front_facing(ps)) { */
        /*     continue; */
        /* } */

        min_p = min_p >> PXLCOORD_SHIFT;
        max_p = max_p >> PXLCOORD_SHIFT;

        float4 weights = {0.25f, 0.25f, 0.25f, 0.25f};
        float4 dsv = vload4(0, &ds[0]);
        int4 Px = vload4(0, &Pxa[0]), Py = vload4(0, &Pya[0]);

        int4 Dy = Px - Px.ywxz;
        int4 Dx = Py.ywxz - Py;

        int2 dm = {Py.w - Py.x, Px.x - Px.w};
        int2 om = idot(dm, (int2){Px.x, Py.x});

        int4 Ox = idot4(Dx, Dy, Px, Py);

        for (int y = min_p.y; y < max_p.y; ++y) {
            for (int x = min_p.x; x < max_p.x; ++x) {

                int2 tp = ((int2){x,y} << PXLCOORD_SHIFT) + (1 << (PXLCOORD_SHIFT));

                int4 Tx = tp.xxxx;
                int4 Ty = tp.yyyy;

                int4 V = (idot4(Dx, Dy, Tx, Ty) - Ox >= 0);
                
                int4 M = (idot(dm, tp) - om  > 0).xxxx ? (int4){1,1,0,0} : (int4){0,0,1,1};

                if (all( V || M )) {

                    LOCK(locks[y][x]);
                    
                    float depth = dot(dsv, weights);
                        
                    if (depths[y][x] > depth) {
                        colors[y][x] = c;
                        depths[y][x] = depth;
                    }

                    UNLOCK(locks[y][x]);
                }
            }
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (next == -2) {
        framebuffer[fb_id] = (float4){1,0,0,1};
    } else {
        float4 c = colors[l.y][l.x];
        float  d = depths[l.y][l.x];

        framebuffer[fb_id] = (float4){c.x, c.y, c.z, d};
        //framebuffer[fb_id] = (float4){c.y, c.z, c.x, d};
    }
}

