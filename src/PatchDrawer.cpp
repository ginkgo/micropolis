#include "PatchDrawer.h"

#include "Projection.h"
#include "Config.h"

namespace Reyes
{
    void bound_n_split(const BezierPatch& patch, const Projection& projection,
                       PatchDrawer& patch_drawer)
    {
        BBox box;

        calc_bbox(patch, box);

        vec2 size;
        bool cull;

        projection.bound(box, size, cull);
    
        if (cull) return;

        int s = config.bound_n_split_limit();

        if (box.min.z < 0 && size.x < s && size.y < s) patch_drawer.draw_patch(patch);
        else {
            BezierPatch p0, p1;
            mat4 proj;
            projection.calc_projection(proj);
            pisplit_patch(patch, p0, p1, proj);
            bound_n_split(p0, projection, patch_drawer);
            bound_n_split(p1, projection, patch_drawer);
        }
    }
}
