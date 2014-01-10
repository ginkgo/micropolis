
#ifndef UTILITY_H
#define UTILITY_H

#ifdef __CL_VERSION_2_0
#warning replace prefix_sum() with work_group_scan_*_*()
#endif

// Calculate (inclusive) prefix sum
inline int prefix_sum(size_t lid, size_t size, int v, local int* pad)
{
    pad[lid] = v;

    // up-sweep
    for (size_t i = 1; i <= size/2; i <<= 1) {
        barrier(CLK_LOCAL_MEM_FENCE);

        if (lid < (size/2)/i) {
            size_t ai = (2*lid+1)*i-1;
            size_t bi = (2*lid+2)*i-1;
            pad[bi] += pad[ai];
        }
    }

    // down-sweep
    for (size_t i = size/2; i >= 2; i >>= 1) {
        barrier(CLK_LOCAL_MEM_FENCE);

        if (lid < size/i-1) {
            size_t ai = i*(lid+1)-1;
            size_t bi = ai + i/2;

            pad[bi] += pad[ai];
        }
    }
        
    barrier(CLK_LOCAL_MEM_FENCE);
    return pad[lid];
}

inline float4 eval_patch(const global float4* patch_buffer,
                         size_t patch_id, float2 t)
{
    float2 s = 1 - t;
    float4 p = (float4)(0,0,0,0);
    
    float v = s.x*s.x*s.x;
    p += v * (1*s.y*s.y*s.y) * patch_buffer[patch_id * 16 + 0];
    p += v * (3*t.y*s.y*s.y) * patch_buffer[patch_id * 16 + 1];
    p += v * (3*t.y*t.y*s.y) * patch_buffer[patch_id * 16 + 2];
    p += v * (1*t.y*t.y*t.y) * patch_buffer[patch_id * 16 + 3];
    
    v = 3*s.x*s.x*t.x;
    p += v * (1*s.y*s.y*s.y) * patch_buffer[patch_id * 16 + 4];
    p += v * (3*t.y*s.y*s.y) * patch_buffer[patch_id * 16 + 5];
    p += v * (3*t.y*t.y*s.y) * patch_buffer[patch_id * 16 + 6];
    p += v * (1*t.y*t.y*t.y) * patch_buffer[patch_id * 16 + 7];
    
    v = 3*s.x*t.x*t.x;
    p += v * (1*s.y*s.y*s.y) * patch_buffer[patch_id * 16 + 8];
    p += v * (3*t.y*s.y*s.y) * patch_buffer[patch_id * 16 + 9];
    p += v * (3*t.y*t.y*s.y) * patch_buffer[patch_id * 16 +10];
    p += v * (1*t.y*t.y*t.y) * patch_buffer[patch_id * 16 +11];
    
    v = t.x*t.x*t.x;
    p += v * (1*s.y*s.y*s.y) * patch_buffer[patch_id * 16 +12];
    p += v * (3*t.y*s.y*s.y) * patch_buffer[patch_id * 16 +13];
    p += v * (3*t.y*t.y*s.y) * patch_buffer[patch_id * 16 +14];
    p += v * (1*t.y*t.y*t.y) * patch_buffer[patch_id * 16 +15];

    return p;
}



typedef struct matrix2
{
    float2 m[2];
} matrix2;

typedef struct matrix4
{
    float4 m[4];
} matrix4;



float2 mul_cm2v2(constant const struct matrix2* mat, float2 vec)
{
    return (mat->m[0] * vec.x +
            mat->m[1] * vec.y );
}

float4 mul_cm4v4(constant const struct matrix4* mat, float4 vec)
{
    return (mat->m[0] * vec.x +
            mat->m[1] * vec.y +
            mat->m[2] * vec.z +
            mat->m[3] * vec.w );
}



float2 mul_pm2v2(private const struct matrix2* mat, float2 vec)
{
    return (mat->m[0] * vec.x +
            mat->m[1] * vec.y );
}

float4 mul_pm4v4(private const struct matrix4* mat, float4 vec)
{
    return (mat->m[0] * vec.x +
            mat->m[1] * vec.y +
            mat->m[2] * vec.z +
            mat->m[3] * vec.w );
}



float4 mul_m44v4(float16 mat, float4 vec)
{
    return (float4) (dot(mat.s048C, vec),
                     dot(mat.s159D, vec),
                     dot(mat.s26AE, vec),
                     dot(mat.s37BF, vec));
}


typedef struct projection
{
    matrix4 proj;
    matrix2 screen_matrix;
    float fovy;
    float2 f;
    float near;
    float far;
    int2 screen_size;
} projection;

inline int round_up_div(int n, int d)
{
    return n/d + ((n%d == 0) ? 0 : 1);
}

inline int round_up_by(int n, int d)
{
    return round_up_div(n,d) * n;
}

#endif
