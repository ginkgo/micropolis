/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/



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

#define VIEWPORT_MIN  (VIEWPORT_MIN_PIXEL  << PXLCOORD_SHIFT)
#define VIEWPORT_MAX  ((VIEWPORT_MAX_PIXEL << PXLCOORD_SHIFT) - 1)
#define VIEWPORT_SIZE (VIEWPORT_SIZE_PIXEL << PXLCOORD_SHIFT)

int calc_framebuffer_pos(int2 pxlpos)
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
    return (float4) (dot(mat.s048C, vec),
                     dot(mat.s159D, vec),
                     dot(mat.s26AE, vec),
                     dot(mat.s37BF, vec));

}

size_t calc_grid_pos(size_t nu, size_t nv, size_t patch)
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

    float2 uv = (float2) (nu/(float)PATCH_SIZE, nv/(float)PATCH_SIZE);

    float4 pos = eval_patch(patch, uv);

    pos.x += native_sin(pos.y*15) * 0.04f;
    pos.y += native_sin(pos.x*15) * 0.04f;
    pos.z += native_sin(pos.x*15) * native_sin(pos.y*15) * 0.04f;
    
    float4 p = mul_m44v4(proj, pos);

    int2 coord = (int2)((int)(p.x/p.w * VIEWPORT_SIZE.x/2 + VIEWPORT_SIZE.x/2),
			(int)(p.y/p.w * VIEWPORT_SIZE.y/2 + VIEWPORT_SIZE.y/2));


    int grid_index = calc_grid_pos(nu, nv, patch_id);
    pos_grid[grid_index] = pos;
    pxlpos_grid[grid_index] = coord;
    depth_grid[grid_index] = p.z/p.w;
}


int is_front_facing(const int2 *ps)
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

int is_empty(int2 min, int2 max)
{
    return min.x >= max.x && min.y >= max.y;
}

int calc_block_pos(int u, int v, int patch_id)
{
    return u + v * BLOCKS_PER_LINE + patch_id * BLOCKS_PER_PATCH;
}

int calc_color_grid_pos(int u, int v, int patch_id)
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
    }


    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_local_id(0) == 0 &&  get_local_id(1) == 0) {
        x_min = max(VIEWPORT_MIN.x, x_min);
        y_min = max(VIEWPORT_MIN.y, y_min);
        x_max = min(VIEWPORT_MAX.x, x_max);
        y_max = min(VIEWPORT_MAX.y, y_max);

        int i = calc_block_pos(get_group_id(0), get_group_id(1), get_group_id(2));
        block_index[i] = (int4)(x_min, y_min, x_max, y_max);
    }

    if (is_empty((int2)(x_min, y_min), (int2)(x_max, y_max))) {
        return;
    }

    float3 du = (pos[1] - pos[0] +
                 pos[3] - pos[2]).xyz * 0.5f;
    float3 dv = (pos[2] - pos[0] +
                 pos[3] - pos[1]).xyz * 0.5f;

    float3 n = normalize(cross(dv,du));

    float3 l = normalize((float3)(4,3,8));

    float3 v = -normalize((pos[0]+pos[1]+pos[2]+pos[3]).xyz);

    float4 dc = (float4)(0.8f, 0.05f, 0.01f, 1);
    float4 sc = (float4)(1, 1, 1, 1);
        
    float3 h = normalize(l+v);
        
    float sh = 30.0f;

    float4 c = max(dot(n,l),0.0f) * dc + pow(max(dot(n,h), 0.0f), sh) * sc;

    color_grid[calc_color_grid_pos(nu, nv, patch_id)] = c;


}


int calc_tile_id(int tx, int ty)
{
    return tx + FRAMEBUFFER_SIZE.x/8 * ty;
}


void recover_patch_pos(size_t block_id, size_t lx, size_t ly,
		       private size_t* u, private size_t* v, private size_t* patch)
{
    *patch = block_id / BLOCKS_PER_PATCH;
    size_t local_block_id = block_id % BLOCKS_PER_PATCH;
    size_t bv = local_block_id / BLOCKS_PER_LINE;
    size_t bu = local_block_id % BLOCKS_PER_LINE;

    *u = bv * 8 + lx;
    *v = bu * 8 + ly;
}

int3 idot3 (int3 Ax, int3 Ay, int3 Bx, int3 By)
{
    // return mad24(Ax, Bx, mul24(Ay,  By));
    return Ax * Bx + Ay * By;
}


int4 idot4 (int4 Ax, int4 Ay, int4 Bx, int4 By)
{
    // return mad24(Ax, Bx, mul24(Ay,  By));
    return Ax * Bx + Ay * By;
}


int idot (int2 a, int2 b)
{
    //return mad24(a.x, b.x, mul24(a.y,  b.y));
    return a.x * b.x + a.y * b.y;
}

int inside_triangle(int3 Px, int3 Py, int2 tp, float3 dv, float* depth)
{
    int3 Dx = Py.yzx - Py;
    int3 Dy = Px - Px.yzx;

    /* int CCW = (Dy.x*Dx.z-Dx.x*Dy.z) < 0; */

    /* Dx = CCW ? Dx : -Dx; */
    /* Dy = CCW ? Dy : -Dy; */

    int3 O = idot3(Dx, Dy, Px, Py);
    
    int3 C = (Dx > 0 || (Dx == 0 && Dy > 0)) ? (int3)(-1,-1,-1) : (int3)(0,0,0);

    int3 V = idot3(Dx, Dy, tp.xxx, tp.yyy) - O;

    int success = all(V > C);

    float3 weights = convert_float3(V.yzx) / convert_float3(idot3(Dx.yzx, Dy.yzx, Px, Py) - O.yzx);

    float d = dot(dv, weights);

    *depth = success && d < *depth ? d : *depth;

    return success;
}


__kernel void init_tile_locks (global int* tile_locks)
{
    // Mark all as unlocked. We only have to call this once.
    int pos = get_global_id(0);
    tile_locks[pos] = 1;
}


__kernel void clear_depth_buffer (global float* depth_buffer)
{
    // Mark all as unlocked. We only have to call this once.
    int pos = get_global_id(0);
    depth_buffer[pos] = INFINITY;
}


#define MAX_LOCAL_COORD  ((8<<PXLCOORD_SHIFT) - 1)

__kernel void sample(global const int4* block_index,
		     global const int2* pxlpos_grid,
		     global const float4* color_grid,
		     global const float* depth_grid,
		     volatile global int* tile_locks,
		     global float4* color_buffer,
		     global float* depth_buffer
		     )
{
    local float4 colors[8][8];
    local float depths[8][8];
    volatile local int locks[8][8];

    int2 l = (int2)(get_global_id(0), get_global_id(1));
    int block_id = get_global_id(2);
    int4 block_bound = block_index[block_id];

    if (is_empty(block_bound.xy, block_bound.zx)) {
	return;
    }

    int2 min_tile = block_bound.xy >> (PXLCOORD_SHIFT + 3);
    int2 max_tile = block_bound.zw >> (PXLCOORD_SHIFT + 3);

    int head = all(l == 0);
    

    // Prepare local position
    float4 c;
    int4 Px, Py;
    float4 dv;
    int2 min_gp = VIEWPORT_MAX+1;
    int2 max_gp = VIEWPORT_MIN-1;
    {
    	int Pxa[4], Pya[4];
    	float da[4];

    	size_t patch_id, u, v;
        recover_patch_pos(block_id, l.x, l.y,  &u, &v, &patch_id);
	
    	c = color_grid[calc_color_grid_pos(u, v, patch_id)];

        for (size_t idx = 0; idx < 4; ++idx) {
            size_t p = calc_grid_pos(u+(idx&1), v+(idx>>1), patch_id);

            int2 pxlpos = pxlpos_grid[p];
            Pxa[idx] = pxlpos.x;
            Pya[idx] = pxlpos.y;

            min_gp = min(min_gp, pxlpos);
            max_gp = max(max_gp, pxlpos);

            float depth = depth_grid[p];
            da[idx] = depth;
        }
	
        Px   = vload4(0, &Pxa[0]);
        Py   = vload4(0, &Pya[0]);
        dv = vload4(0, &da[0]);
    }

    locks[l.x][l.y] = 1;
    barrier(CLK_LOCAL_MEM_FENCE);

    for     (int ty = min_tile.y; ty <= max_tile.y; ++ty) {
        for (int tx = min_tile.x; tx <= max_tile.x; ++tx) {
	    int2 o = (int2)(tx*8, ty*8);
	    int2 os = o << PXLCOORD_SHIFT;
            int tile_id = calc_tile_id(tx,ty);
	    int2 fb_pos = l + o;
	    int fb_id = calc_framebuffer_pos(fb_pos);

	    depths[l.x][l.y] = INFINITY;

	    barrier(CLK_LOCAL_MEM_FENCE);
	    
	    int2 min_p = clamp(min_gp - os, 0, MAX_LOCAL_COORD);
	    int2 max_p = clamp(max_gp - os, 0, MAX_LOCAL_COORD);

	    min_p = min_p >> PXLCOORD_SHIFT;
	    max_p = max_p >> PXLCOORD_SHIFT;

	    //printf("%d %d %d %d\n", min_p.x, min_p.y, max_p.x, max_p.y);

	    for (int y = min_p.y; y <= max_p.y; ++y) {
	    	for (int x = min_p.x; x <= max_p.x; ++x) {

	    	    int2 tp = ((int2)(x,y) << PXLCOORD_SHIFT) + os;

	    	    float depth = 1;
	    	    int inside1 = inside_triangle(Px.xyw, Py.xyw, tp, dv.xyw, &depth);
	    	    int inside2 = inside_triangle(Px.xwz, Py.xwz, tp, dv.xwz, &depth);
                
	    	    if (inside1 || inside2) {
	    		while (1) {
	    		    if (atomic_cmpxchg(&(locks[y][x]), 1, 0)) continue;

	    		    //colors[y][x] += 0.2f;

	    		    if (depths[y][x] > depth) {
	    			colors[y][x] = c;
	    			depths[y][x] = depth;
	    		    }
                        
	    		    atomic_xchg(&(locks[y][x]), 1);
	    		    break;
	    		}
	    	    }
	    	}
	    }
	    

	    // rasterize tile

	    if (head) while(atomic_cmpxchg(&(tile_locks[tile_id]), 1, 0));
	    barrier(CLK_LOCAL_MEM_FENCE);

	    if (depths[l.y][l.x] < depth_buffer[fb_id]) {
		depth_buffer[fb_id] = depths[l.y][l.x]; 
		color_buffer[fb_id] = colors[l.y][l.x];
	    } 

	    barrier(CLK_LOCAL_MEM_FENCE);
	    if (head) atomic_xchg(&(tile_locks[tile_id]), 1);
        }
    }    
}
