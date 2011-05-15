#include "Projection.h"

void PerspectiveProjection::calc_projection(mat4& proj) const
{
    proj = glm::perspective<float>(_fovy, _aspect, _near, 1000);
}

void PerspectiveProjection::bound(const BBox& bbox, vec2& size, bool& cull) const
{
    vec2 min,max;

    float n = -bbox.max.z;
    float f = -bbox.min.z;

    if (f < _near) {
        cull = true;
        return;
    }

    n = maximum(_near, n);

    if (bbox.min.x < 0) {
        min.x = bbox.min.x/n * fx + vp.x;
    } else {
        min.x = bbox.min.x/f * fx + vp.x;
    }

    if (bbox.min.y < 0) {
        min.y = bbox.min.y/n * fy + vp.y;
    } else {
        min.y = bbox.min.y/f * fy + vp.y;
    }

    if (bbox.max.x > 0) {
        max.x = bbox.max.x/n * fx + vp.x;
    } else {
        max.x = bbox.max.x/f * fx + vp.x;
    }

    if (bbox.max.y > 0) {
        max.y = bbox.max.y/n * fy + vp.y;
    } else {
        max.y = bbox.max.y/f * fy + vp.y;
    }


    if (min.x > _viewport.x-1 || max.x < 0 ||
        min.y > _viewport.y-1 || max.y < 0 ){
        cull = true;
        return;
    }

    size = max - min;
    cull = false;
}
