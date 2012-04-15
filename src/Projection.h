#ifndef PROJECTION_H
#define PROJECTION_H

#include "common.h"

namespace Reyes {

    class Projection
    {
        public:

        virtual ~Projection() {};

        virtual void calc_projection(mat4& proj) const = 0;
        virtual ivec4 get_viewport() const = 0;
        virtual void bound(const BBox& bbox, vec2& size, bool& cull) const = 0;
    };

    class PerspectiveProjection : public Projection
    {
		float _fovy;
        float _near;
		float _aspect;
        ivec2 _viewport;

        float fy,fx;
        vec2 vp;

        public:

        PerspectiveProjection(float fovy, float hither, ivec2 viewport);

        virtual ~PerspectiveProjection() {};

        void calc_projection(mat4& proj) const;    
        ivec4 get_viewport() const;    
        void bound(const BBox& bbox, vec2& size, bool& cull) const;
    };

}

#endif
