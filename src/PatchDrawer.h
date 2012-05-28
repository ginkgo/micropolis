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


#ifndef PATCHDRAWER_H
#define PATCHDRAWER_H

#include "Patch.h"

namespace Reyes
{

    class Projection;

    struct PatchDrawer
    {
        virtual ~PatchDrawer() {};

        virtual void prepare() = 0;
        virtual void finish() = 0;
        
        virtual void set_projection(const Reyes::Projection& projection) = 0;
        virtual void draw_patch(const BezierPatch& patch) = 0;
    };

    void bound_n_split(const BezierPatch& patch, 
                       const Reyes::Projection& projection,
                       PatchDrawer& patch_drawer);
}

#endif
