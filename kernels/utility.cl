
#include "utility.h"

// Calculate (inclusive) prefix sum
int prefix_sum(size_t lid, size_t size, int v, local int* pad)
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

float4 eval_bezier_patch(const global float4* patch_buffer,
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


float4 eval_gregory_patch(const global float4* patch_buffer,
                                 size_t patch_id, float2 t)
{
    float2 s = 1 - t;
    float4 p = (float4)(0,0,0,0);

    size_t o = patch_id * 20;
    const global float4* P = patch_buffer + o;
    
    float4 F0,F1,F2,F3;
    {
        float u = t.x;
        float v = t.y;

        F0 = (  u  *P[12]+  v  *P[16])/(  u+v);
        F1 = ((1-u)*P[17]+  v  *P[13])/(1-u+v);
        F2 = ((1-u)*P[14]+(1-v)*P[18])/(2-u-v);
        F3 = (  u  *P[19]+(1-v)*P[15])/(1+u-v);

        // Make sure no NaNs from division by zero propagate into final value
        F0 = select(F0, (float4)(0), isnan(F0));
        F1 = select(F1, (float4)(0), isnan(F1));
        F2 = select(F2, (float4)(0), isnan(F2));
        F3 = select(F3, (float4)(0), isnan(F3));
    }
    
    float v = s.x*s.x*s.x;
    p += v * (1*s.y*s.y*s.y) * P[ 0];
    p += v * (3*t.y*s.y*s.y) * P[ 4];
    p += v * (3*t.y*t.y*s.y) * P[ 9];
    p += v * (1*t.y*t.y*t.y) * P[ 1];
    
    v = 3*s.x*s.x*t.x;
    p += v * (1*s.y*s.y*s.y) * P[ 8];
    p += v * (3*t.y*s.y*s.y) * F0;
    p += v * (3*t.y*t.y*s.y) * F1;
    p += v * (1*t.y*t.y*t.y) * P[ 5];
    
    v = 3*s.x*t.x*t.x;
    p += v * (1*s.y*s.y*s.y) * P[ 7];
    p += v * (3*t.y*s.y*s.y) * F3;
    p += v * (3*t.y*t.y*s.y) * F2;
    p += v * (1*t.y*t.y*t.y) * P[10];
    
    v = t.x*t.x*t.x;
    p += v * (1*s.y*s.y*s.y) * P[ 3];
    p += v * (3*t.y*s.y*s.y) * P[11];
    p += v * (3*t.y*t.y*s.y) * P[ 6];
    p += v * (1*t.y*t.y*t.y) * P[ 2];

    return p;
}

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

int round_up_div(int n, int d)
{
    return n/d + ((n%d == 0) ? 0 : 1);
}

int round_up_by(int n, int d)
{
    return round_up_div(n,d) * n;
}
