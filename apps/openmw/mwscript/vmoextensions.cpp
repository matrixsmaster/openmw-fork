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

#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

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

#include <extern/xswrapper/xswrapper.hpp>
#include <extern/xswrapper/eventsink.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/soundmanager.hpp"
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

                // load "seed" image
                osg::ref_ptr<osg::Texture2D> tex2 (new osg::Texture2D(rsys->getImageManager()->getImage("textures\\menu_morrowind.dds")));
                tex2->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
                tex2->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);
                rsys->getSceneManager()->applyFilterSettings(tex2);

                // prepare display node
                SceneUtil::PositionAttitudeTransform* ptr = obj.getRefData().getBaseNode();
                osg::ref_ptr<osg::Geode> myNode = new osg::Geode();

                // read arguments
                Interpreter::Type_Float offX = runtime[0].mFloat;
                runtime.pop();
                Interpreter::Type_Float offY = runtime[0].mFloat;
                runtime.pop();
                Interpreter::Type_Float offZ = runtime[0].mFloat;
                runtime.pop();
                Interpreter::Type_Float szX = runtime[0].mFloat;
                runtime.pop();
                Interpreter::Type_Float szY = runtime[0].mFloat;
                runtime.pop();
                Interpreter::Type_Float szZ = runtime[0].mFloat;
                runtime.pop();
                printf("Creating VMO with offset (%.2f, %.2f, %.2f) and size (%.2f, %.2f, %.2f)\n",offX,offY,offZ,szX,szY,szZ);

                // and add it to the scene
                myNode->addDrawable(new osg::ShapeDrawable(new osg::Box(osg::Vec3f(offX,offY,offZ),szX,szY,szZ)));
                myNode->setName("VirtualMachineObject");
                myNode->setDataVariance(osg::Object::DYNAMIC);
                ptr->addChild(myNode.get());

                // set new (seed) texture properties
                osg::StateSet* stateset = myNode->getOrCreateStateSet();
                osg::TexEnv* pTexEnv = new osg::TexEnv();

                pTexEnv->setMode(osg::TexEnv::REPLACE);
                stateset->setTextureAttributeAndModes(0, pTexEnv, osg::StateAttribute::ON);
                stateset->setTextureAttributeAndModes(0, tex2, osg::StateAttribute::ON);

                // create the VMO controller (this will lead to VM initialization)
                osg::ref_ptr<NifOsg::VMOController> controller(new NifOsg::VMOController(0, 1));

                // start up the VMO controller
                controller->setSource(std::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));
                myNode->setUpdateCallback(controller);

                // start sound
                MWBase::Environment::get().getSoundManager()->streamVMO();
            }
        };

        template <class R>
        class OpSetEventSink : public Interpreter::Opcode0
        {
        public:
            virtual void execute(Interpreter::Runtime &runtime)
            {
                MWWorld::Ptr obj = R()(runtime);

                Interpreter::Type_Integer en = runtime[0].mInteger;
                runtime.pop();
                printf("Event sink enable: %d\n",en);

                if (!en) {
                    MWBase::Environment::get().getInputManager()->setEventSinks(NULL);

                } else {
                    if (MyGUI::InputManager::getInstance().isModalAny())
                        return;

                    if (MWBase::Environment::get().getWindowManager()->isGuiMode())
                    {
                        if (MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_Console)
                            MWBase::Environment::get().getWindowManager()->popGuiMode();
                    }

                    MWBase::Environment::get().getInputManager()->setEventSinks(wrapperGetEventSinks());
                }
            }
        };

        void installOpcodes(Interpreter::Interpreter& interpreter)
        {
            interpreter.installSegment5(Compiler::VMO::opcodeEnableVMO, new OpEnableVMO<ImplicitRef>);
            interpreter.installSegment5(Compiler::VMO::opcodeEnableVMOExplicit, new OpEnableVMO<ExplicitRef>);
            interpreter.installSegment5(Compiler::VMO::opcodeSetEventSink, new OpSetEventSink<ImplicitRef>);
            interpreter.installSegment5(Compiler::VMO::opcodeSetEventSinkExplicit, new OpSetEventSink<ExplicitRef>);
        }
    }
}
