/* -*-c++-*-
 *
 * Copyright (C) 2007 Tim Moore
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

#ifndef SIMGEAR_STATEATTRIBUTEFACTORY_HXX
#define SIMGEAR_STATEATTRIBUTEFACTORY_HXX 1

#include <OpenThreads/Mutex>
#include <osg/ref_ptr>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/ShadeModel>
#include <osg/Texture2D>
#include <osg/TexEnv>

// Return read-only instances of common OSG state attributes.
namespace simgear
{
class StateAttributeFactory : public osg::Referenced {
public:
    // Alpha test > .01
    osg::AlphaFunc* getStandardAlphaFunc() { return _standardAlphaFunc.get(); }
    // alpha source, 1 - alpha destination
    osg::BlendFunc* getStandardBlendFunc() { return _standardBlendFunc.get(); }
    // modulate
    osg::TexEnv* getStandardTexEnv() { return _standardTexEnv.get(); }
    osg::ShadeModel* getSmoothShadeModel() { return _smooth.get(); }
    osg::ShadeModel* getFlatShadeModel() { return _flat.get(); }
    // White, repeating texture
    osg::Texture2D* getWhiteTexture() { return _whiteTexture.get(); }
    static StateAttributeFactory* instance();
protected:
    StateAttributeFactory();
    osg::ref_ptr<osg::AlphaFunc> _standardAlphaFunc;
    osg::ref_ptr<osg::ShadeModel> _smooth;
    osg::ref_ptr<osg::ShadeModel> _flat;
    osg::ref_ptr<osg::BlendFunc> _standardBlendFunc;
    osg::ref_ptr<osg::TexEnv> _standardTexEnv;
    osg::ref_ptr<osg::Texture2D> _whiteTexture;
    static osg::ref_ptr<StateAttributeFactory> _theInstance;
    static OpenThreads::Mutex _instanceMutex;
};
}
#endif