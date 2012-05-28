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
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.        *
\******************************************************************************/


#include "Projection.h"
#include "Config.h"


Reyes::PerspectiveProjection::PerspectiveProjection(float fovy, float hither, ivec2 viewport):
    _fovy(fovy), _near(hither), 
    _aspect(float(viewport.x)/viewport.y),
    _viewport(viewport) 
{
    fy = 1.0f/(float)tan(_fovy * M_PI / 360);
    fx = fy / _aspect;

    vp = vec2(_viewport.x/2.0, _viewport.y/2.0);

    fx *= vp.x;
    fy *= vp.y;
};

void Reyes::PerspectiveProjection::calc_projection(mat4& proj) const
{
    proj = glm::perspective<float>(_fovy, _aspect, _near, 1000);
}

ivec4 Reyes::PerspectiveProjection::get_viewport() const
{
    return ivec4(0,0,_viewport.x, _viewport.y);
}

void Reyes::PerspectiveProjection::bound(const BBox& bbox, vec2& size, bool& cull) const
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


    if (min.x > _viewport.x-1 + config.cull_ribbon() || 
	max.x < -config.cull_ribbon() ||
	min.y > _viewport.y-1 + config.cull_ribbon()|| 
	max.y < -config.cull_ribbon() ){
        cull = true;
        return;
    }

    size = max - min;
    cull = false;
}
