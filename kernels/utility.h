
#ifndef UTILITY_H
#define UTILITY_H

#ifdef __CL_VERSION_2_0
#warning replace prefix_sum() with work_group_scan_*_*()
#endif

// Calculates (inclusive) prefix sum
int prefix_sum(size_t lid, size_t size, int v, local int* pad);

float4 eval_bezier_patch(const global float4* patch_buffer, size_t patch_id, float2 t);
float4 eval_gregory_patch(const global float4* patch_buffer, size_t patch_id, float2 t);

typedef struct matrix2
{
    float2 m[2];
} matrix2;

typedef struct matrix4
{
    float4 m[4];
} matrix4;


float2 mul_cm2v2(constant const struct matrix2* mat, float2 vec);
float4 mul_cm4v4(constant const struct matrix4* mat, float4 vec);
float2 mul_pm2v2(private const struct matrix2* mat, float2 vec);
float4 mul_pm4v4(private const struct matrix4* mat, float4 vec);
float4 mul_m44v4(float16 mat, float4 vec);


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

int round_up_div(int n, int d);
int round_up_by(int n, int d);

#endif
