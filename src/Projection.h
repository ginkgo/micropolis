#ifndef PROJECTION_H
#define PROJECTION_H

#include "common.h"

class Projection
{
    public:

    virtual void calc_projection(mat4& proj) const = 0;
    virtual void bound(const BBox& bbox, vec2& size, bool& cull) const = 0;
};

class PerspectiveProjection : public Projection
{
    float _near, _fovy, _aspect;
    ivec2 _viewport;

    float fy,fx;
    vec2 vp;

    public:

    PerspectiveProjection(float fovy, float near, ivec2 viewport):
        _near(near), _fovy(fovy), 
        _aspect(float(viewport.x)/viewport.y),
        _viewport(viewport) 
    {
        fy = 1/tan(_fovy * M_PI / 360);
        fx = fy / _aspect;

        vp = vec2(_viewport.x/2.0, _viewport.y/2.0);

        fx *= vp.x;
        fy *= vp.y;
    };

    void calc_projection(mat4& proj) const;    
    void bound(const BBox& bbox, vec2& size, bool& cull) const;
};

#endif
