// ModelRegistry.hxx -- interface to the OSG model registry
//
// Copyright (C) 2005-2007 Mathias Froehlich 
// Copyright (C) 2007  Tim Moore <timoore@redhat.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#include "ModelRegistry.hxx"

#include <algorithm>

#include <OpenThreads/ScopedLock>

#include <osg/observer_ptr>
#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/NodeCallback>
#include <osg/Switch>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osgDB/Archive>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osgDB/SharedStateManager>
#include <osgUtil/Optimizer>

#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/SGStateAttributeVisitor.hxx>
#include <simgear/scene/util/SGTextureStateAttributeVisitor.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>

using namespace std;
using namespace osg;
using namespace osgUtil;
using namespace osgDB;
using namespace simgear;

// Little helper class that holds an extra reference to a
// loaded 3d model.
// Since we clone all structural nodes from our 3d models,
// the database pager will only see one single reference to
// top node of the model and expire it relatively fast.
// We attach that extra reference to every model cloned from
// a base model in the pager. When that cloned model is deleted
// this extra reference is deleted too. So if there are no
// cloned models left the model will expire.
namespace {
class SGDatabaseReference : public Observer {
public:
  SGDatabaseReference(Referenced* referenced) :
    mReferenced(referenced)
  { }
  virtual void objectDeleted(void*)
  {
    mReferenced = 0;
  }
private:
  ref_ptr<Referenced> mReferenced;
};

// Visitor for 
class SGTextureUpdateVisitor : public SGTextureStateAttributeVisitor {
public:
  SGTextureUpdateVisitor(const FilePathList& pathList) :
    mPathList(pathList)
  { }
  Texture2D* textureReplace(int unit,
                            StateSet::RefAttributePair& refAttr)
  {
    Texture2D* texture;
    texture = dynamic_cast<Texture2D*>(refAttr.first.get());
    if (!texture)
      return 0;
    
    ref_ptr<Image> image = texture->getImage(0);
    if (!image)
      return 0;

    // The currently loaded file name
    string fullFilePath = image->getFileName();
    // The short name
    string fileName = getSimpleFileName(fullFilePath);
    // The name that should be found with the current database path
    string fullLiveryFile = findFileInPath(fileName, mPathList);
    // If they are identical then there is nothing to do
    if (fullLiveryFile == fullFilePath)
      return 0;

    image = readImageFile(fullLiveryFile);
    if (!image)
      return 0;

    CopyOp copyOp(CopyOp::DEEP_COPY_ALL & ~CopyOp::DEEP_COPY_IMAGES);
    texture = static_cast<Texture2D*>(copyOp(texture));
    if (!texture)
      return 0;
    texture->setImage(image.get());
    return texture;
  }
  virtual void apply(StateSet* stateSet)
  {
    if (!stateSet)
      return;

    // get a copy that we can safely modify the statesets values.
    StateSet::TextureAttributeList attrList;
    attrList = stateSet->getTextureAttributeList();
    for (unsigned unit = 0; unit < attrList.size(); ++unit) {
      StateSet::AttributeList::iterator i = attrList[unit].begin();
      while (i != attrList[unit].end()) {
        Texture2D* texture = textureReplace(unit, i->second);
        if (texture) {
          stateSet->removeTextureAttribute(unit, i->second.first.get());
          stateSet->setTextureAttribute(unit, texture, i->second.second);
          stateSet->setTextureMode(unit, GL_TEXTURE_2D, StateAttribute::ON);
        }
        ++i;
      }
    }
  }

private:
  FilePathList mPathList;
};

class SGTexCompressionVisitor : public SGTextureStateAttributeVisitor {
public:
  virtual void apply(int, StateSet::RefAttributePair& refAttr)
  {
    Texture2D* texture;
    texture = dynamic_cast<Texture2D*>(refAttr.first.get());
    if (!texture)
      return;

    // Hmm, true??
    texture->setDataVariance(osg::Object::STATIC);

    Image* image = texture->getImage(0);
    if (!image)
      return;

    int s = image->s();
    int t = image->t();

    if (s <= t && 32 <= s) {
      SGSceneFeatures::instance()->setTextureCompression(texture);
    } else if (t < s && 32 <= t) {
      SGSceneFeatures::instance()->setTextureCompression(texture);
    }
  }
};

class SGTexDataVarianceVisitor : public SGTextureStateAttributeVisitor {
public:
  virtual void apply(int, StateSet::RefAttributePair& refAttr)
  {
    Texture* texture;
    texture = dynamic_cast<Texture*>(refAttr.first.get());
    if (!texture)
      return;
    
    texture->setDataVariance(Object::STATIC);
  }

  virtual void apply(StateSet* stateSet)
  {
    if (!stateSet)
      return;
    SGTextureStateAttributeVisitor::apply(stateSet);
    stateSet->setDataVariance(Object::STATIC);
  }
};

class SGAcMaterialCrippleVisitor : public SGStateAttributeVisitor {
public:
  virtual void apply(StateSet::RefAttributePair& refAttr)
  {
    Material* material;
    material = dynamic_cast<Material*>(refAttr.first.get());
    if (!material)
      return;
    material->setColorMode(Material::AMBIENT_AND_DIFFUSE);
  }
};

// Work around an OSG bug - the file loaders don't use the file path
// in options while the file is being loaded.

struct OptionsPusher {
    FilePathList localPathList;
    bool validOptions;
    OptionsPusher(const ReaderWriter::Options* options):
        validOptions(false)
    {
        if (!options)
            return;
        Registry* registry = Registry::instance();
        localPathList = registry->getDataFilePathList();
        const FilePathList& regPathList = registry->getDataFilePathList();
        const FilePathList& optionsPathList = options->getDatabasePathList();
        for (FilePathList::const_iterator iter = optionsPathList.begin();
             iter != optionsPathList.end();
             ++iter) {
            if (find(regPathList.begin(), regPathList.end(), *iter)
                == regPathList.end())
                localPathList.push_back(*iter);
        }
        // Save the current Registry path list and install the augmented one.
        localPathList.swap(registry->getDataFilePathList());
        validOptions = true;
    }
    ~OptionsPusher()
    {
        // Restore the old path list
        if (validOptions)
            localPathList.swap(Registry::instance()->getDataFilePathList());
    }
};
} // namespace

ReaderWriter::ReadResult
ModelRegistry::readImage(const string& fileName,
                         const ReaderWriter::Options* opt)
{
    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(readerMutex);
    CallbackMap::iterator iter
        = imageCallbackMap.find(getFileExtension(fileName));
    // XXX Workaround for OSG plugin bug
    {
        OptionsPusher pusher(opt);
        if (iter != imageCallbackMap.end() && iter->second.valid())
            return iter->second->readImage(fileName, opt);
        string absFileName = findDataFile(fileName);
        if (!fileExists(absFileName)) {
            SG_LOG(SG_IO, SG_ALERT, "Cannot find image file \""
                   << fileName << "\"");
            return ReaderWriter::ReadResult::FILE_NOT_FOUND;
        }

        Registry* registry = Registry::instance();
        ReaderWriter::ReadResult res;
        res = registry->readImageImplementation(absFileName, opt);
        if (res.loadedFromCache())
            SG_LOG(SG_IO, SG_INFO, "Returning cached image \""
                   << res.getImage()->getFileName() << "\"");
        else
            SG_LOG(SG_IO, SG_INFO, "Reading image \""
                   << res.getImage()->getFileName() << "\"");

        return res;
    }
}


osg::Node* DefaultCachePolicy::find(const string& fileName,
                                    const ReaderWriter::Options* opt)
{
    Registry* registry = Registry::instance();
    osg::Node* cached
        = dynamic_cast<Node*>(registry->getFromObjectCache(fileName));
    if (cached)
        SG_LOG(SG_IO, SG_INFO, "Got cached model \""
               << fileName << "\"");
    else
        SG_LOG(SG_IO, SG_INFO, "Reading model \""
               << fileName << "\"");
    return cached;
}

void DefaultCachePolicy::addToCache(const string& fileName,
                                    osg::Node* node)
{
    Registry::instance()->addEntryToObjectCache(fileName, node);
}

// Optimizations we don't use:
// Don't use this one. It will break animation names ...
// opts |= osgUtil::Optimizer::REMOVE_REDUNDANT_NODES;
//
// opts |= osgUtil::Optimizer::REMOVE_LOADED_PROXY_NODES;
// opts |= osgUtil::Optimizer::COMBINE_ADJACENT_LODS;
// opts |= osgUtil::Optimizer::CHECK_GEOMETRY;
// opts |= osgUtil::Optimizer::SPATIALIZE_GROUPS;
// opts |= osgUtil::Optimizer::COPY_SHARED_NODES;
// opts |= osgUtil::Optimizer::TESSELATE_GEOMETRY;
// opts |= osgUtil::Optimizer::OPTIMIZE_TEXTURE_SETTINGS;

OptimizeModelPolicy::OptimizeModelPolicy(const string& extension) :
    _osgOptions(Optimizer::SHARE_DUPLICATE_STATE
                | Optimizer::MERGE_GEOMETRY
                | Optimizer::FLATTEN_STATIC_TRANSFORMS
                | Optimizer::TRISTRIP_GEOMETRY)
{
}

osg::Node* OptimizeModelPolicy::optimize(osg::Node* node,
                                         const string& fileName,
                                         const osgDB::ReaderWriter::Options* opt)
{
    osgUtil::Optimizer optimizer;
    optimizer.optimize(node, _osgOptions);

    // Make sure the data variance of sharable objects is set to
    // STATIC so that textures will be globally shared.
    SGTexDataVarianceVisitor dataVarianceVisitor;
    node->accept(dataVarianceVisitor);
      
    SGTexCompressionVisitor texComp;
    node->accept(texComp);
    return node;
}

osg::Node* DefaultCopyPolicy::copy(osg::Node* model, const string& fileName,
                    const osgDB::ReaderWriter::Options* opt)
{
    // Add an extra reference to the model stored in the database.
    // That it to avoid expiring the object from the cache even if it is still
    // in use. Note that the object cache will think that a model is unused
    // if the reference count is 1. If we clone all structural nodes here
    // we need that extra reference to the original object
    SGDatabaseReference* databaseReference;
    databaseReference = new SGDatabaseReference(model);
    CopyOp::CopyFlags flags = CopyOp::DEEP_COPY_ALL;
    flags &= ~CopyOp::DEEP_COPY_TEXTURES;
    flags &= ~CopyOp::DEEP_COPY_IMAGES;
    flags &= ~CopyOp::DEEP_COPY_STATESETS;
    flags &= ~CopyOp::DEEP_COPY_STATEATTRIBUTES;
    flags &= ~CopyOp::DEEP_COPY_ARRAYS;
    flags &= ~CopyOp::DEEP_COPY_PRIMITIVES;
    // This will safe display lists ...
    flags &= ~CopyOp::DEEP_COPY_DRAWABLES;
    flags &= ~CopyOp::DEEP_COPY_SHAPES;
    osg::Node* res = CopyOp(flags)(model);
    res->addObserver(databaseReference);

    // Update liveries
    SGTextureUpdateVisitor liveryUpdate(opt->getDatabasePathList());
    res->accept(liveryUpdate);
    return res;
}

string OSGSubstitutePolicy::substitute(const string& name,
                                       const ReaderWriter::Options* opt)
{
    string fileSansExtension = getNameLessExtension(name);
    string osgFileName = fileSansExtension + ".osg";
    string absFileName = findDataFile(osgFileName, opt);
    return absFileName;
}

ModelRegistry::ModelRegistry() :
    _defaultCallback(new DefaultCallback(""))
{
}

void
ModelRegistry::addImageCallbackForExtension(const string& extension,
                                            Registry::ReadFileCallback* callback)
{
    imageCallbackMap.insert(CallbackMap::value_type(extension, callback));
}

void
ModelRegistry::addNodeCallbackForExtension(const string& extension,
                                           Registry::ReadFileCallback* callback)
{
    nodeCallbackMap.insert(CallbackMap::value_type(extension, callback));
}

ref_ptr<ModelRegistry> ModelRegistry::instance;

ModelRegistry* ModelRegistry::getInstance()

{
    if (!instance.valid())
        instance = new ModelRegistry;
    return instance.get();
}

ReaderWriter::ReadResult
ModelRegistry::readNode(const string& fileName,
                        const ReaderWriter::Options* opt)
{
    OpenThreads::ScopedLock<OpenThreads::ReentrantMutex> lock(readerMutex);
    // XXX Workaround for OSG plugin bug.
    OptionsPusher pusher(opt);
    Registry* registry = Registry::instance();
    ReaderWriter::ReadResult res;
    Node* cached = 0;
    CallbackMap::iterator iter
        = nodeCallbackMap.find(getFileExtension(fileName));
    if (iter != nodeCallbackMap.end() && iter->second.valid())
        return iter->second->readNode(fileName, opt);
    return _defaultCallback->readNode(fileName, opt);
}

class SGReadCallbackInstaller {
public:
  SGReadCallbackInstaller()
  {
    // XXX I understand why we want this, but this seems like a weird
    // place to set this option.
    Referenced::setThreadSafeReferenceCounting(true);

    Registry* registry = Registry::instance();
    ReaderWriter::Options* options = new ReaderWriter::Options;
    int cacheOptions = ReaderWriter::Options::CACHE_ALL;
    options->
      setObjectCacheHint((ReaderWriter::Options::CacheHintOptions)cacheOptions);
    registry->setOptions(options);
    registry->getOrCreateSharedStateManager()->
      setShareMode(SharedStateManager::SHARE_STATESETS);
    registry->setReadFileCallback(ModelRegistry::getInstance());
  }
};

static SGReadCallbackInstaller readCallbackInstaller;

// we get optimal geometry from the loader.
struct ACOptimizePolicy : public OptimizeModelPolicy {
    ACOptimizePolicy(const string& extension)  :
        OptimizeModelPolicy(extension)
    {
        _osgOptions &= ~Optimizer::TRISTRIP_GEOMETRY;
    }
};

struct ACProcessPolicy {
    ACProcessPolicy(const string& extension) {}
    Node* process(Node* node, const string& filename,
                  const ReaderWriter::Options* opt)
    {
        Matrix m(1, 0, 0, 0,
                 0, 0, 1, 0,
                 0, -1, 0, 0,
                 0, 0, 0, 1);
        // XXX Does there need to be a Group node here to trick the
        // optimizer into optimizing the static transform?
        osg::Group* root = new Group;
        MatrixTransform* transform = new MatrixTransform;
        root->addChild(transform);
        
        transform->setDataVariance(Object::STATIC);
        transform->setMatrix(m);
        transform->addChild(node);
        // Ok, this step is questionable.
        // It is there to have the same visual appearance of ac objects for the
        // first cut. Osg's ac3d loader will correctly set materials from the
        // ac file. But the old plib loader used GL_AMBIENT_AND_DIFFUSE for the
        // materials that in effect igored the ambient part specified in the
        // file. We emulate that for the first cut here by changing all
        // ac models here. But in the long term we should use the
        // unchanged model and fix the input files instead ...
        SGAcMaterialCrippleVisitor matCriple;
        root->accept(matCriple);
        return root;
    }
};

typedef ModelRegistryCallback<ACProcessPolicy, DefaultCachePolicy,
                              ACOptimizePolicy, DefaultCopyPolicy,
                              OSGSubstitutePolicy> ACCallback;

namespace
{
ModelRegistryCallbackProxy<ACCallback> g_acRegister("ac");
}   