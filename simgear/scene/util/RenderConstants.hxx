/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Tim Moore
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef SIMGEAR_RENDER_CONSTANTS_HXX
#define SIMGEAR_RENDER_CONSTANTS_HXX
// Constants used in the scene graph, both node masks and render bins.
namespace simgear {

enum NodeMask {
    TERRAIN_BIT = (1 << 0),
    MAINMODEL_BIT = (1 << 1),
    CASTSHADOW_BIT = (1 << 2),
    RECEIVESHADOW_BIT = (1 << 3),
    GUI_BIT = (1 << 4),
    PANEL2D_BIT = (1 << 5),
    PICK_BIT = (1 << 6)
    // Different classes of lights turned on by node masks
};
}
#endif