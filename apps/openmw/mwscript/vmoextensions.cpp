#include "vmoextensions.hpp"

#include <cstdio>
#include <cstdlib>

#include <osg/PolygonOffset>
#include <osg/Texture2D>
#include <osg/TexGen>
#include <osg/TexEnv>
#include <osg/Material>
#include <osg/Depth>
#include <osg/PositionAttitudeTransform>
#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>
#include <osg/Geode>
#include <osg/ShapeDrawable>

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>
#include <components/compiler/locals.hpp>
#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/misc/rng.hpp>
#include <components/nifosg/controller.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/fallback/fallback.hpp>
#include <components/nifosg/controller.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwmechanics/aicast.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace VMO
    {
        template <class R>
        class OpEnableVMO : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime)
            {
                MWWorld::Ptr obj = R()(runtime);
                Resource::ResourceSystem* rsys = MWBase::Environment::get().getWorld()->getResourceSystem();
#if 0
                int num = 51;
                std::vector<osg::ref_ptr<osg::Texture2D> > textures;
                for (int i=0; i<num; ++i)
                {
                    std::ostringstream texname;
                    texname << "textures/0_DELETE_ME/anim-" << i << ".png";
                    printf("Loading up %s\n",texname.str().c_str());

                    osg::ref_ptr<osg::Texture2D> tex2 (new osg::Texture2D(rsys->getImageManager()->getImage(texname.str())));
                    tex2->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
                    tex2->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);
                    rsys->getSceneManager()->applyFilterSettings(tex2);
                    tex2->setName(texname.str());
                    textures.push_back(tex2);
                    printf("-> %u\n",tex2->getImage()->getImageSizeInBytes());
                }
#else
                osg::ref_ptr<osg::Texture2D> tex2 (new osg::Texture2D(rsys->getImageManager()->getImage("textures/0_DELETE_ME/anim-1.png")));
                tex2->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
                tex2->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);
                rsys->getSceneManager()->applyFilterSettings(tex2);
#endif
                SceneUtil::PositionAttitudeTransform* ptr = obj.getRefData().getBaseNode();
                osg::ref_ptr<osg::Geode> myNode = new osg::Geode();

                myNode->addDrawable(new osg::ShapeDrawable(new osg::Box(osg::Vec3f(0.0f,0.0f,0.0f),50.f)));
                ptr->addChild(myNode.get());

                osg::StateSet* stateset = myNode->getOrCreateStateSet();
                osg::TexEnv* pTexEnv = new osg::TexEnv();

                pTexEnv->setMode(osg::TexEnv::REPLACE);
                stateset->setTextureAttributeAndModes(0, pTexEnv, osg::StateAttribute::ON);
                stateset->setTextureAttributeAndModes(0, tex2, osg::StateAttribute::ON);

                osg::ref_ptr<NifOsg::VMOController> controller(new NifOsg::VMOController(0, 1));

                controller->setSource(std::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));
                myNode->setUpdateCallback(controller);
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5(Compiler::VMO::opcodeEnableVMO, new OpEnableVMO<ImplicitRef>);
            interpreter.installSegment5(Compiler::VMO::opcodeEnableVMOExplicit, new OpEnableVMO<ExplicitRef>);
        }
    }
}
