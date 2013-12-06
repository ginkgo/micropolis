


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
