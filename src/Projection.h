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


#ifndef PROJECTION_H
#define PROJECTION_H

#include "common.h"

namespace Reyes {

    class Projection
    {
		float _fovy;
        float _near;
		float _aspect;
        ivec2 _viewport;

        float fy,fx;
        vec2 vp;

        public:

        Projection(float fovy, float hither, ivec2 viewport);

        ~Projection() {};

        void calc_projection(mat4& proj) const;    
        void calc_projection_with_aspect_correction(mat4& proj) const;
        void calc_screen_matrix(mat2& screen_matrix) const;
        ivec4 get_viewport() const;    
        void bound(const BBox& bbox, vec2& size, bool& cull) const;
        
        float near() const { return _near; }
        float far()  const { return 1000;  }
        vec2 f() const { return vec2(fx,fy); }
        vec2 viewport() const { return vec2(_viewport.x, _viewport.y); }
        ivec2 viewport_i() const { return _viewport; }
        float fovy() const { return _fovy; }
    };

}

#endif
