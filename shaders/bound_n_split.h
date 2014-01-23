

uniform float split_limit;
uniform uint max_split_depth;

uniform samplerBuffer patches;

uniform float near;
uniform float far;
uniform vec2 proj_f;
uniform vec2 screen_size;
uniform float cull_ribbon;

uniform mat4 mv;
uniform mat4 proj;
uniform mat3 screen_matrix;


vec3 eval_patch(vec2 t, int patch_id)
{
    vec2 s = 1 - t;

    mat4 B = outerProduct(vec4(s.x*s.x*s.x, 3*s.x*s.x*t.x, 3*s.x*t.x*t.x, t.x*t.x*t.x),
                          vec4(s.y*s.y*s.y, 3*s.y*s.y*t.y, 3*s.y*t.y*t.y, t.y*t.y*t.y));
    
    vec3 sum = vec3(0,0,0);
    for (int u = 0; u < 4; ++u) {
        for (int v = 0; v < 4; ++v) {
            sum += B[v][u] * texelFetch(patches, u + v * 4 + patch_id * 16).xyz;
        }
    }

    return sum;
}

bool intersects_frustum(vec3 pmin, vec3 pmax)
{
    vec2 smin,smax;

    float n = -pmax.z;
    float f = -pmin.z;

    if (f < near || n > far) {
        return false;
    }

    n = max(near, n);

    if (pmin.x < 0) {
        smin.x = pmin.x/n * proj_f.x + screen_size.x * 0.5;
    } else {
        smin.x = pmin.x/f * proj_f.x + screen_size.x * 0.5;
    }

    if (pmin.y < 0) {
        smin.y = pmin.y/n * proj_f.y + screen_size.y * 0.5;
    } else {
        smin.y = pmin.y/f * proj_f.y + screen_size.y * 0.5;
    }

    if (pmax.x > 0) {
        smax.x = pmax.x/n * proj_f.x + screen_size.x * 0.5;
    } else {
        smax.x = pmax.x/f * proj_f.x + screen_size.x * 0.5;
    }

    if (pmax.y > 0) {
        smax.y = pmax.y/n * proj_f.y + screen_size.y * 0.5;
    } else {
        smax.y = pmax.y/f * proj_f.y + screen_size.y * 0.5;
    }


    if (smin.x > screen_size.x-1 + cull_ribbon || smax.x < -cull_ribbon ||
        smin.y > screen_size.y-1 + cull_ribbon || smax.y < -cull_ribbon ){
        return false;
    }

    return true;
}


// Returns 0b00000CBA
// A ... draw
// B ... split
// C ... split direction, 0=horizontal 1=vertical
#define RES 3
#define CULL 0
#define DRAW 1
#define HSPLIT 2
#define VSPLIT 6
uint bound (uint rpid, vec2 rmin, vec2 rmax, uint rdepth)
{
    // Calculate bounding box and max u/v length of patch 
    vec2 ppos[RES][RES];
    
    vec3 bbox_min = vec3( infinity);
    vec3 bbox_max = vec3(-infinity);

    if (rdepth >= max_split_depth-1) return CULL;
    
    for (uint u = 0; u < RES; ++u) {
        for (uint v = 0; v < RES; ++v) {
            vec2 uv = mix(rmin, rmax, vec2(u * (1.0f / (RES-1)), v * (1.0f / (RES-1))));

            vec4 p = mv * vec4(eval_patch(mix(rmin,rmax,uv), int(rpid)),1);

            bbox_min = min(bbox_min, p.xyz);
            bbox_max = max(bbox_max, p.xyz);

            vec4 pp = proj * p;
            ppos[v][u] = mat2(screen_matrix) * (pp.xy / pp.w);
        }
    }

    if (!intersects_frustum(bbox_min, bbox_max)) {
        return CULL;
    } else if (bbox_min.z < 0 && bbox_max.z > 0) {
        return ((rmax.x - rmin.x) < (rmax.y - rmin.y)) ? VSPLIT : HSPLIT;
    } else {

        float hlen=0, vlen=0;
        for (uint i = 0; i < RES; ++i) {
            float h = 0, v = 0;
            for (uint j = 0; j < RES-1; ++j) {
                h += distance(ppos[i][j], ppos[i][j+1]);
                v += distance(ppos[j][i], ppos[j+1][i]);
            }
        
            hlen = max(h, hlen);
            vlen = max(v, vlen);
        }

        if (hlen <= split_limit && vlen <= split_limit) {
            return DRAW;
        } else {
            return (hlen < vlen) ? VSPLIT : HSPLIT;
        }
    }

    // unreachable
}
