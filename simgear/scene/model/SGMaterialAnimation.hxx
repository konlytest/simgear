
// animation.hxx - classes to manage model animation.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef _SG_MATERIALANIMATION_HXX
#define _SG_MATERIALANIMATION_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include "animation.hxx"

//////////////////////////////////////////////////////////////////////
// Material animation
//////////////////////////////////////////////////////////////////////

class SGMaterialAnimation : public SGAnimation {
public:
  SGMaterialAnimation(const SGPropertyNode* configNode,
                      SGPropertyNode* modelRoot);
  virtual osg::Group* createAnimationGroup(osg::Group& parent);
  virtual void install(osg::Node& node);
private:
  struct ColorSpec;
  struct PropSpec;
  class MaterialVisitor;
  class UpdateCallback;
};

#endif // _SG_MATERIALANIMATION_HXX