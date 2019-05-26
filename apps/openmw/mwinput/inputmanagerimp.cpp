#include "inputmanagerimp.hpp"

#include <osgViewer/ViewerEventHandlers>

#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>

#include <SDL_version.h>

#include <components/debug/debuglog.hpp>
#include <components/sdlutil/sdlinputwrapper.hpp>
#include <components/sdlutil/sdlvideowrapper.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/controlsstate.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

namespace MWInput
{
    InputManager::InputManager(
            SDL_Window* window,
            osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler,
            osgViewer::ScreenCaptureHandler::CaptureOperation *screenCaptureOperation,
            const std::string& userFile, bool userFileExists, bool grab)
        : mWindow(window)
        , mWindowVisible(true)
        , mViewer(viewer)
        , mScreenCaptureHandler(screenCaptureHandler)
        , mScreenCaptureOperation(screenCaptureOperation)
        , mPlayer(nullptr)
        , mInputManager(nullptr)
        , mVideoWrapper(nullptr)
        , mUserFile(userFile)
        , mDragDrop(false)
        , mGrabCursor(Settings::Manager::getBool("grab cursor", "Input"))
        , mInvertY(Settings::Manager::getBool("invert y axis", "Input"))
        , mControlsDisabled(false)
        , mCameraSensitivity(Settings::Manager::getFloat("camera sensitivity", "Input"))
        , mCameraYMultiplier(Settings::Manager::getFloat("camera y multiplier", "Input"))
        , mPreviewPOVDelay(0.f)
        , mTimeIdle(0.f)
        , mMouseLookEnabled(false)
        , mGuiCursorEnabled(true)
        , mNowDetecting(false)
        , mOverencumberedMessageDelay(0.f)
        , mGuiCursorX(0)
        , mGuiCursorY(0)
        , mMouseWheel(0)
        , mUserFileExists(userFileExists)
        , mAlwaysRunActive(Settings::Manager::getBool("always run", "Input"))
        , mSneakToggles(Settings::Manager::getBool("toggle sneak", "Input"))
        , mSneaking(false)
        , mAttemptJump(false)
        , mInvUiScalingFactor(1.f)
        , mFakeDeviceID(1)
        , mEventSinkEnabled(false)
    {
        mInputManager = new SDLUtil::InputWrapper(window, viewer, grab);
        mInputManager->setMouseEventCallback(this);
        mInputManager->setKeyboardEventCallback(this);
        mInputManager->setWindowEventCallback(this);

        mVideoWrapper = new SDLUtil::VideoWrapper(window, viewer);
        mVideoWrapper->setGammaContrast(Settings::Manager::getFloat("gamma", "Video"),
                                        Settings::Manager::getFloat("contrast", "Video"));

        std::string file = userFileExists ? userFile : "";
        //TODO: load the last saved map

        loadKeyDefaults();

        for(int i = 0; i < A_Last; ++i) mBindsEnabled[i] = true;

        mControlSwitch["playercontrols"]      = true;
        mControlSwitch["playerfighting"]      = true;
        mControlSwitch["playerjumping"]       = true;
        mControlSwitch["playerlooking"]       = true;
        mControlSwitch["playermagic"]         = true;
        mControlSwitch["playerviewswitch"]    = true;
        mControlSwitch["vanitymode"]          = true;

        float uiScale = Settings::Manager::getFloat("scaling factor", "GUI");
        if (uiScale != 0.f)
            mInvUiScalingFactor = 1.f / uiScale;

        int w,h;
        SDL_GetWindowSize(window, &w, &h);

        mGuiCursorX = mInvUiScalingFactor * w / 2.f;
        mGuiCursorY = mInvUiScalingFactor * h / 2.f;
    }

    void InputManager::clear()
    {
        // Enable all controls
        for(std::map<std::string, bool>::iterator it = mControlSwitch.begin(); it != mControlSwitch.end(); ++it)
            it->second = true;
    }

    InputManager::~InputManager()
    {
        //TODO: save the current map

        delete mInputManager;

        delete mVideoWrapper;
    }

    bool InputManager::isWindowVisible()
    {
        return mWindowVisible;
    }

    void InputManager::setPlayerControlsEnabled(bool enabled)
    {
        int playerChannels[] = {A_AutoMove, A_AlwaysRun, A_ToggleWeapon,
                                A_ToggleSpell, A_Rest, A_QuickKey1, A_QuickKey2,
                                A_QuickKey3, A_QuickKey4, A_QuickKey5, A_QuickKey6,
                                A_QuickKey7, A_QuickKey8, A_QuickKey9, A_QuickKey10,
                                A_Use, A_Journal};

        for(size_t i = 0; i < sizeof(playerChannels)/sizeof(playerChannels[0]); i++)
            mBindsEnabled[playerChannels[i]] = enabled;
    }

    void InputManager::handleGuiArrowKey(int action)
    {
        if (SDL_IsTextInputActive())
            return;

        MyGUI::KeyCode key;
        switch(action)
        {
        case A_MoveLeft:
            key = MyGUI::KeyCode::ArrowLeft;
            break;
        case A_MoveRight:
            key = MyGUI::KeyCode::ArrowRight;
            break;
        case A_MoveForward:
            key = MyGUI::KeyCode::ArrowUp;
            break;
        case A_MoveBackward:
        default:
            key = MyGUI::KeyCode::ArrowDown;
            break;
        }

        MWBase::Environment::get().getWindowManager()->injectKeyPress(key, 0, false);
    }

    void InputManager::channelChanged(int action)
    {
        resetIdleTime();

        if (mDragDrop && action != A_GameMenu && action != A_Inventory)
            return;

        int currentValue = mBindsState[action].second;
        int previousValue = mBindsState[action].first;

        if (mControlSwitch["playercontrols"])
        {
            if (action == A_Use)
            {
                MWMechanics::DrawState_ state = MWBase::Environment::get().getWorld()->getPlayer().getDrawState();
                mPlayer->setAttackingOrSpell(currentValue != 0 && state != MWMechanics::DrawState_Nothing);
            }
            else if (action == A_Jump)
                mAttemptJump = (currentValue == 1 && previousValue == 0);
        }

        if (currentValue == 1)
        {
            // trigger action activated
            switch(action)
            {
            case A_GameMenu:
                toggleMainMenu();
                break;
            case A_Screenshot:
                screenshot();
                break;
            case A_Inventory:
                toggleInventory();
                break;
            case A_Console:
                toggleConsole();
                break;
            case A_Activate:
                resetIdleTime();
                activate();
                break;
            case A_MoveLeft:
            case A_MoveRight:
            case A_MoveForward:
            case A_MoveBackward:
                handleGuiArrowKey(action);
                break;
            case A_Journal:
                toggleJournal();
                break;
            case A_AutoMove:
                toggleAutoMove();
                break;
            case A_AlwaysRun:
                toggleWalking();
                break;
            case A_ToggleWeapon:
                toggleWeapon();
                break;
            case A_Rest:
                rest();
                break;
            case A_ToggleSpell:
                toggleSpell();
                break;
            case A_QuickKey1:
                quickKey(1);
                break;
            case A_QuickKey2:
                quickKey(2);
                break;
            case A_QuickKey3:
                quickKey(3);
                break;
            case A_QuickKey4:
                quickKey(4);
                break;
            case A_QuickKey5:
                quickKey(5);
                break;
            case A_QuickKey6:
                quickKey(6);
                break;
            case A_QuickKey7:
                quickKey(7);
                break;
            case A_QuickKey8:
                quickKey(8);
                break;
            case A_QuickKey9:
                quickKey(9);
                break;
            case A_QuickKey10:
                quickKey(10);
                break;
            case A_QuickKeysMenu:
                showQuickKeysMenu();
                break;
            case A_ToggleHUD:
                MWBase::Environment::get().getWindowManager()->toggleHud();
                break;
            case A_ToggleDebug:
                MWBase::Environment::get().getWindowManager()->toggleDebugWindow();
                break;
            case A_QuickSave:
                quickSave();
                break;
            case A_QuickLoad:
                quickLoad();
                break;
            case A_CycleSpellLeft:
                if (checkAllowedToUseItems())
                    MWBase::Environment::get().getWindowManager()->cycleSpell(false);
                break;
            case A_CycleSpellRight:
                if (checkAllowedToUseItems())
                    MWBase::Environment::get().getWindowManager()->cycleSpell(true);
                break;
            case A_CycleWeaponLeft:
                if (checkAllowedToUseItems())
                    MWBase::Environment::get().getWindowManager()->cycleWeapon(false);
                break;
            case A_CycleWeaponRight:
                if (checkAllowedToUseItems())
                    MWBase::Environment::get().getWindowManager()->cycleWeapon(true);
                break;
            case A_Sneak:
                if (mSneakToggles)
                {
                    toggleSneaking();
                }
                break;
            }
        }
    }

    void InputManager::updateCursorMode()
    {
        bool grab = !MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_MainMenu)
             && MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Console;

        bool was_relative = mInputManager->getMouseRelative();
        bool is_relative = !MWBase::Environment::get().getWindowManager()->isGuiMode();

        // don't keep the pointer away from the window edge in gui mode
        // stop using raw mouse motions and switch to system cursor movements
        mInputManager->setMouseRelative(is_relative);

        //we let the mouse escape in the main menu
        mInputManager->setGrabPointer(grab && (mGrabCursor || is_relative));

        //we switched to non-relative mode, move our cursor to where the in-game
        //cursor is
        if ( !is_relative && was_relative != is_relative )
        {
            mInputManager->warpMouse(static_cast<int>(mGuiCursorX/mInvUiScalingFactor), static_cast<int>(mGuiCursorY/mInvUiScalingFactor));
        }
    }

    bool InputManager::checkAllowedToUseItems() const
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        if (player.getClass().getNpcStats(player).isWerewolf())
        {
            // Cannot use items or spells while in werewolf form
            MWBase::Environment::get().getWindowManager()->messageBox("#{sWerewolfRefusal}");
            return false;
        }
        return true;
    }

    void InputManager::update(float dt, bool disableControls, bool disableEvents)
    {
        mControlsDisabled = disableControls;

        mInputManager->setMouseVisible(MWBase::Environment::get().getWindowManager()->getCursorVisible());

        mInputManager->capture(disableEvents);

        if (mControlsDisabled)
        {
            updateCursorMode();
            return;
        }

        updateCursorMode();

        // Disable movement in Gui mode
        if (!(MWBase::Environment::get().getWindowManager()->isGuiMode()
            || MWBase::Environment::get().getStateManager()->getState() != MWBase::StateManager::State_Running))
        {
            // Configure player movement according to keyboard input. Actual movement will
            // be done in the physics system.
            if (mControlSwitch["playercontrols"])
            {
                bool triedToMove = false;

                // keyboard movement
                if (triedToMove) resetIdleTime();

                if (actionIsActive(A_MoveLeft) && !actionIsActive(A_MoveRight))
                {
                    triedToMove = true;
                    mPlayer->setLeftRight(-1);
                }
                else if (actionIsActive(A_MoveRight) && !actionIsActive(A_MoveLeft))
                {
                    triedToMove = true;
                    mPlayer->setLeftRight(1);
                }

                if (actionIsActive(A_MoveForward) && !actionIsActive(A_MoveBackward))
                {
                    triedToMove = true;
                    mPlayer->setAutoMove(false);
                    mPlayer->setForwardBackward(1);
                }
                else if (actionIsActive(A_MoveBackward) && !actionIsActive(A_MoveForward))
                {
                    triedToMove = true;
                    mPlayer->setAutoMove(false);
                    mPlayer->setForwardBackward(-1);
                }
                else if (mPlayer->getAutoMove())
                {
                    triedToMove = true;
                    mPlayer->setForwardBackward(1);
                }

                if (!mSneakToggles)
                {
                    mPlayer->setSneak(actionIsActive(A_Sneak));
                }

                if (mAttemptJump && mControlSwitch["playerjumping"])
                {
                    mPlayer->setUpDown(1);
                    triedToMove = true;
                    mOverencumberedMessageDelay = 0.f;
                }

                if (mAlwaysRunActive)
                    mPlayer->setRunState(!actionIsActive(A_Run));
                else
                    mPlayer->setRunState(actionIsActive(A_Run));

                // if player tried to start moving, but can't(due to being overencumbered), display a notification.
                if (triedToMove)
                {
                    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
                    mOverencumberedMessageDelay -= dt;
                    if (player.getClass().getEncumbrance(player) > player.getClass().getCapacity(player))
                    {
                        mPlayer->setAutoMove(false);
                        if (mOverencumberedMessageDelay <= 0)
                        {
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage59}");
                            mOverencumberedMessageDelay = 1.0;
                        }
                    }
                }

                if (mControlSwitch["playerviewswitch"]) {

                    if (actionIsActive(A_TogglePOV)) {
                        if (mPreviewPOVDelay <= 0.5 &&
                            (mPreviewPOVDelay += dt) > 0.5)
                        {
                            mPreviewPOVDelay = 1.f;
                            MWBase::Environment::get().getWorld()->togglePreviewMode(true);
                        }
                    } else {
                        //disable preview mode
                        MWBase::Environment::get().getWorld()->togglePreviewMode(false);
                        if (mPreviewPOVDelay > 0.f && mPreviewPOVDelay <= 0.5) {
                            MWBase::Environment::get().getWorld()->togglePOV();
                        }
                        mPreviewPOVDelay = 0.f;
                    }
                }
            }
            if (actionIsActive(A_MoveForward) ||
                actionIsActive(A_MoveBackward) ||
                actionIsActive(A_MoveLeft) ||
                actionIsActive(A_MoveRight) ||
                actionIsActive(A_Jump) ||
                actionIsActive(A_Sneak) ||
                actionIsActive(A_TogglePOV))
            {
                resetIdleTime();
            } else {
                updateIdleTime(dt);
            }
        }
        mAttemptJump = false; // Can only jump on first frame input is on
    }

    void InputManager::setDragDrop(bool dragDrop)
    {
        mDragDrop = dragDrop;
    }

    void InputManager::changeInputMode(bool guiMode)
    {
        mGuiCursorEnabled = guiMode;
        mMouseLookEnabled = !guiMode;
        if (guiMode)
            MWBase::Environment::get().getWindowManager()->showCrosshair(false);
        MWBase::Environment::get().getWindowManager()->setCursorVisible(guiMode);
        // if not in gui mode, the camera decides whether to show crosshair or not.
    }

    void InputManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        bool changeRes = false;

        for(Settings::CategorySettingVector::const_iterator it = changed.begin();
        it != changed.end(); ++it)
        {
            if (it->first == "Input" && it->second == "invert y axis")
                mInvertY = Settings::Manager::getBool("invert y axis", "Input");

            if (it->first == "Input" && it->second == "camera sensitivity")
                mCameraSensitivity = Settings::Manager::getFloat("camera sensitivity", "Input");

            if (it->first == "Input" && it->second == "grab cursor")
                mGrabCursor = Settings::Manager::getBool("grab cursor", "Input");

            if (it->first == "Video" && (
                    it->second == "resolution x"
                    || it->second == "resolution y"
                    || it->second == "fullscreen"
                    || it->second == "window border"))
                changeRes = true;

            if (it->first == "Video" && it->second == "vsync")
                mVideoWrapper->setSyncToVBlank(Settings::Manager::getBool("vsync", "Video"));

            if (it->first == "Video" && (it->second == "gamma" || it->second == "contrast"))
                mVideoWrapper->setGammaContrast(Settings::Manager::getFloat("gamma", "Video"),
                                                Settings::Manager::getFloat("contrast", "Video"));
        }

        if (changeRes)
        {
            mVideoWrapper->setVideoMode(Settings::Manager::getInt("resolution x", "Video"),
                                        Settings::Manager::getInt("resolution y", "Video"),
                                        Settings::Manager::getBool("fullscreen", "Video"),
                                        Settings::Manager::getBool("window border", "Video"));

            // We should reload TrueType fonts to fit new resolution
            MWBase::Environment::get().getWindowManager()->loadUserFonts();
        }
    }

    bool InputManager::getControlSwitch(const std::string& sw)
    {
        return mControlSwitch[sw];
    }

    void InputManager::toggleControlSwitch(const std::string& sw, bool value)
    {
        if (mControlSwitch[sw] == value) {
            return;
        }

        // Note: 7 switches at all, if-else is relevant
        if (sw == "playercontrols" && !value) {
            mPlayer->setLeftRight(0);
            mPlayer->setForwardBackward(0);
            mPlayer->setAutoMove(false);
            mPlayer->setUpDown(0);

        } else if (sw == "playerjumping" && !value) {
            // FIXME:  maybe crouching at this time
            mPlayer->setUpDown(0);

        } else if (sw == "vanitymode") {
            MWBase::Environment::get().getWorld()->allowVanityMode(value);

        } else if (sw == "playerlooking") {
            MWBase::Environment::get().getWorld()->togglePlayerLooking(value);

        }
        mControlSwitch[sw] = value;
    }

    void InputManager::fireUpBind(SDL_Scancode key, int state)
    {
        for (auto &i : mBinds) {
            if (i.second != key) continue;
            if (!mBindsEnabled[i.first]) return; //control disabled

            mBindsState[i.first].first = mBindsState[i.first].second;
            mBindsState[i.first].second = state;

            if (!mControlsDisabled) channelChanged(i.first);
        }
    }

    void InputManager::keyPressed(const SDL_KeyboardEvent &arg)
    {
        if (mEventSinkEnabled) {
            mEventSinkEnabled = mEventSinks.keydown(&arg);
            return;
        }

        // HACK: to make Morrowind's default keybinding for the console work without printing an extra "^" upon closing
        // This assumes that SDL_TextInput events always come *after* the key event
        // (which is somewhat reasonable, and hopefully true for all SDL platforms)
        if (mBinds[A_Console] == arg.keysym.scancode && MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_Console)
            SDL_StopTextInput();

        bool consumed = false;
        MyGUI::KeyCode::Enum kc = mInputManager->SDL2MGKeyCode(arg.keysym.sym);
        if (kc != MyGUI::KeyCode::None && !mNowDetecting)
        {
            consumed = MWBase::Environment::get().getWindowManager()->injectKeyPress(kc, 0, arg.repeat);

            // Little trick to check if key is printable
            if (SDL_IsTextInputActive() &&  (!(SDLK_SCANCODE_MASK & arg.keysym.sym) && std::isprint(arg.keysym.sym)))
                consumed = true;

            setPlayerControlsEnabled(!consumed);
        }

        if (arg.repeat) return;

        if (mNowDetecting) {
            keyBindingDetected(arg.keysym.scancode);
            return;
        }

        if (!mControlsDisabled && !consumed)
            fireUpBind(arg.keysym.scancode, 1); //pressed
    }

    void InputManager::textInput(const SDL_TextInputEvent &arg)
    {
        MyGUI::UString ustring(&arg.text[0]);
        MyGUI::UString::utf32string utf32string = ustring.asUTF32();
        for(MyGUI::UString::utf32string::const_iterator it = utf32string.begin(); it != utf32string.end(); ++it)
            MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::None, *it);
    }

    void InputManager::keyReleased(const SDL_KeyboardEvent &arg)
    {
        if (mEventSinkEnabled) {
            mEventSinkEnabled = mEventSinks.keyup(&arg);
            return;
        }

        MyGUI::KeyCode::Enum kc = mInputManager->SDL2MGKeyCode(arg.keysym.sym);

        if (!mNowDetecting)
            setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(kc));

        fireUpBind(arg.keysym.scancode, 0); //released
    }

    void InputManager::mousePressed(const SDL_MouseButtonEvent &arg, Uint8 id)
    {
        bool guiMode = false;

        if (mEventSinkEnabled) {
            mEventSinkEnabled = mEventSinks.mousedown(&arg);
            return;
        }

        if (id == SDL_BUTTON_LEFT || id == SDL_BUTTON_RIGHT) // MyGUI only uses these mouse events
        {
            guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
            guiMode = MyGUI::InputManager::getInstance().injectMousePress(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), sdlButtonToMyGUI(id)) && guiMode;
            if (MyGUI::InputManager::getInstance().getMouseFocusWidget() != 0)
            {
                MyGUI::Button* b = MyGUI::InputManager::getInstance().getMouseFocusWidget()->castType<MyGUI::Button>(false);
                if (b && b->getEnabled() && id == SDL_BUTTON_LEFT)
                {
                    MWBase::Environment::get().getWindowManager()->playSound("Menu Click");
                }
            }
            MWBase::Environment::get().getWindowManager()->setCursorActive(true);
        }

        setPlayerControlsEnabled(!guiMode);

        // Don't trigger any mouse bindings while in settings menu, otherwise rebinding controls becomes impossible
        if (MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Settings)
            fireUpBind((SDL_Scancode)(arg.button + OMWI_MOUSECODE_BASE), 1); //pressed

        else if (mNowDetecting)
            mouseButtonBindingDetected(arg.button);

    }

    void InputManager::mouseReleased(const SDL_MouseButtonEvent &arg, Uint8 id)
    {
        if (mNowDetecting) return;

        if (mEventSinkEnabled) {
            mEventSinkEnabled = mEventSinks.mouseup(&arg);
            return;
        }

        bool guiMode = MWBase::Environment::get().getWindowManager()->isGuiMode();
        guiMode = MyGUI::InputManager::getInstance().injectMouseRelease(static_cast<int>(mGuiCursorX), static_cast<int>(mGuiCursorY), sdlButtonToMyGUI(id)) && guiMode;

        if (mNowDetecting) return; // don't allow same mouseup to bind as initiated bind

        setPlayerControlsEnabled(!guiMode);
        fireUpBind((SDL_Scancode)(arg.button + OMWI_MOUSECODE_BASE), 0); //released
    }

    void InputManager::mouseMoved(const SDLUtil::MouseMotionEvent &arg )
    {
        resetIdleTime();

        if (mEventSinkEnabled) {
            mEventSinkEnabled = mEventSinks.mouse(&arg);
            return;
        }

        if (mGuiCursorEnabled)
        {
            // We keep track of our own mouse position, so that moving the mouse while in
            // game mode does not move the position of the GUI cursor
            mGuiCursorX = static_cast<float>(arg.x) * mInvUiScalingFactor;
            mGuiCursorY = static_cast<float>(arg.y) * mInvUiScalingFactor;

            mMouseWheel = int(arg.z);

            MyGUI::InputManager::getInstance().injectMouseMove( int(mGuiCursorX), int(mGuiCursorY), mMouseWheel);
            // FIXME: inject twice to force updating focused widget states(tooltips) resulting from changing the viewport by scroll wheel
            MyGUI::InputManager::getInstance().injectMouseMove( int(mGuiCursorX), int(mGuiCursorY), mMouseWheel);

            MWBase::Environment::get().getWindowManager()->setCursorActive(true);
        }

        if (mMouseLookEnabled && !mControlsDisabled)
        {
            resetIdleTime();

            float x = arg.xrel * mCameraSensitivity * (1.0f/256.f);
            float y = arg.yrel * mCameraSensitivity * (1.0f/256.f) * (mInvertY ? -1 : 1) * mCameraYMultiplier;

            float rot[3];
            rot[0] = -y;
            rot[1] = 0.0f;
            rot[2] = -x;

            // Only actually turn player when we're not in vanity mode
            if (!MWBase::Environment::get().getWorld()->vanityRotateCamera(rot))
            {
                mPlayer->yaw(x);
                mPlayer->pitch(y);
            }

            if (arg.zrel && mControlSwitch["playerviewswitch"] && mControlSwitch["playercontrols"]) //Check to make sure you are allowed to zoomout and there is a change
            {
                MWBase::Environment::get().getWorld()->changeVanityModeScale(static_cast<float>(arg.zrel));

                if (Settings::Manager::getBool("allow third person zoom", "Input"))
                    MWBase::Environment::get().getWorld()->setCameraDistance(static_cast<float>(arg.zrel), true, true);
            }
        }
    }

    void InputManager::windowFocusChange(bool have_focus)
    {
    }

    void InputManager::windowVisibilityChange(bool visible)
    {
        mWindowVisible = visible;
    }

    void InputManager::windowResized(int x, int y)
    {
        Settings::Manager::setInt("resolution x", "Video", x);
        Settings::Manager::setInt("resolution y", "Video", y);

        MWBase::Environment::get().getWindowManager()->windowResized(x, y);
    }

    void InputManager::windowClosed()
    {
        MWBase::Environment::get().getStateManager()->requestQuit();
    }

    void InputManager::toggleMainMenu()
    {
        if (MyGUI::InputManager::getInstance().isModalAny()) {
            MWBase::Environment::get().getWindowManager()->exitCurrentModal();
            return;
        }

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode()) //No open GUIs, open up the MainMenu
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_MainMenu);
        }
        else //Close current GUI
        {
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
        }
    }

    void InputManager::quickLoad() {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickLoad();
    }

    void InputManager::quickSave() {
        if (!MyGUI::InputManager::getInstance().isModalAny())
            MWBase::Environment::get().getStateManager()->quickSave();
    }
    void InputManager::toggleSpell()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        // Not allowed before the magic window is accessible
        if (!mControlSwitch["playermagic"] || !mControlSwitch["playercontrols"])
            return;

        if (!checkAllowedToUseItems())
            return;

        // Not allowed if no spell selected
        MWWorld::InventoryStore& inventory = mPlayer->getPlayer().getClass().getInventoryStore(mPlayer->getPlayer());
        if (MWBase::Environment::get().getWindowManager()->getSelectedSpell().empty() &&
            inventory.getSelectedEnchantItem() == inventory.end())
            return;

        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(mPlayer->getPlayer()))
            return;

        MWMechanics::DrawState_ state = mPlayer->getDrawState();
        if (state == MWMechanics::DrawState_Weapon || state == MWMechanics::DrawState_Nothing)
            mPlayer->setDrawState(MWMechanics::DrawState_Spell);
        else
            mPlayer->setDrawState(MWMechanics::DrawState_Nothing);
    }

    void InputManager::toggleWeapon()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        // Not allowed before the inventory window is accessible
        if (!mControlSwitch["playerfighting"] || !mControlSwitch["playercontrols"])
            return;

        // We want to interrupt animation only if attack is preparing, but still is not triggered
        // Otherwise we will get a "speedshooting" exploit, when player can skip reload animation by hitting "Toggle Weapon" key twice
        if (MWBase::Environment::get().getMechanicsManager()->isAttackPreparing(mPlayer->getPlayer()))
            mPlayer->setAttackingOrSpell(false);
        else if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(mPlayer->getPlayer()))
            return;

        MWMechanics::DrawState_ state = mPlayer->getDrawState();
        if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
            mPlayer->setDrawState(MWMechanics::DrawState_Weapon);
        else
            mPlayer->setDrawState(MWMechanics::DrawState_Nothing);
    }

    void InputManager::rest()
    {
        if (!mControlSwitch["playercontrols"])
            return;

        if (!MWBase::Environment::get().getWindowManager()->getRestEnabled() || MWBase::Environment::get().getWindowManager()->isGuiMode())
            return;

        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Rest); //Open rest GUI

    }

    void InputManager::screenshot()
    {
        bool regularScreenshot = true;

        std::string settingStr;

        settingStr = Settings::Manager::getString("screenshot type","Video");
        regularScreenshot = settingStr.size() == 0 || settingStr.compare("regular") == 0;

        if (regularScreenshot)
        {
            mScreenCaptureHandler->setFramesToCapture(1);
            mScreenCaptureHandler->captureNextFrame(*mViewer);
        }
        else
        {
            osg::ref_ptr<osg::Image> screenshot(new osg::Image);

            if (MWBase::Environment::get().getWorld()->screenshot360(screenshot.get(),settingStr))
            {
                (*mScreenCaptureOperation) (*(screenshot.get()),0);
                // FIXME: mScreenCaptureHandler->getCaptureOperation() causes crash for some reason
            }
        }
    }

    void InputManager::toggleInventory()
    {
        if (!mControlSwitch["playercontrols"])
            return;

        if (MyGUI::InputManager::getInstance().isModalAny())
            return;

        // Toggle between game mode and inventory mode
        if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Inventory);
        else
        {
            MWGui::GuiMode mode = MWBase::Environment::get().getWindowManager()->getMode();
            if (mode == MWGui::GM_Inventory || mode == MWGui::GM_Container)
                MWBase::Environment::get().getWindowManager()->popGuiMode();
        }

        // .. but don't touch any other mode, except container.
    }

    void InputManager::toggleConsole()
    {
        if (MyGUI::InputManager::getInstance().isModalAny())
            return;

        // Switch to console mode no matter what mode we are currently
        // in, except of course if we are already in console mode
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_Console)
                MWBase::Environment::get().getWindowManager()->popGuiMode();
            else
                MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Console);
        }
        else
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Console);
    }

    void InputManager::toggleJournal()
    {
        if (!mControlSwitch["playercontrols"])
            return;
        if (MyGUI::InputManager::getInstance().isModalAny())
            return;

        if (MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Journal
                && MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_MainMenu
                && MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_Settings
                && MWBase::Environment::get().getWindowManager()->getJournalAllowed())
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Journal);
        }
        else if (MWBase::Environment::get().getWindowManager()->containsMode(MWGui::GM_Journal))
        {
            MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Journal);
        }
    }

    void InputManager::quickKey(int index)
    {
        if (!mControlSwitch["playercontrols"])
            return;
        if (!checkAllowedToUseItems())
            return;

        if (!MWBase::Environment::get().getWindowManager()->isGuiMode())
            MWBase::Environment::get().getWindowManager()->activateQuickKey(index);
    }

    void InputManager::showQuickKeysMenu()
    {
        if (!MWBase::Environment::get().getWindowManager()->isGuiMode()
                && MWBase::Environment::get().getWorld()->getGlobalFloat("chargenstate")==-1)
        {
            if (!checkAllowedToUseItems())
                return;

            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_QuickKeysMenu);

        }
        else if (MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_QuickKeysMenu) {
            while(MyGUI::InputManager::getInstance().isModalAny()) { //Handle any open Modal windows
                MWBase::Environment::get().getWindowManager()->exitCurrentModal();
            }
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode(); //And handle the actual main window
        }
    }

    void InputManager::activate()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode())
        {
            if (!SDL_IsTextInputActive())
                MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::Return, 0, false);
        }
        else if (mControlSwitch["playercontrols"])
            mPlayer->activate();
    }

    void InputManager::toggleAutoMove()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;

        if (mControlSwitch["playercontrols"])
            mPlayer->setAutoMove(!mPlayer->getAutoMove());
    }

    void InputManager::toggleWalking()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;
        mAlwaysRunActive = !mAlwaysRunActive;

        Settings::Manager::setBool("always run", "Input", mAlwaysRunActive);
    }

    void InputManager::toggleSneaking()
    {
        if (MWBase::Environment::get().getWindowManager()->isGuiMode()) return;
        mSneaking = !mSneaking;
        mPlayer->setSneak(mSneaking);
    }

    void InputManager::resetIdleTime()
    {
        if (mTimeIdle < 0)
            MWBase::Environment::get().getWorld()->toggleVanityMode(false);
        mTimeIdle = 0.f;
    }

    void InputManager::updateIdleTime(float dt)
    {
        static const float vanityDelay = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                .find("fVanityDelay")->mValue.getFloat();
        if (mTimeIdle >= 0.f)
            mTimeIdle += dt;
        if (mTimeIdle > vanityDelay) {
            MWBase::Environment::get().getWorld()->toggleVanityMode(true);
            mTimeIdle = -1.f;
        }
    }

    bool InputManager::actionIsActive(int id)
    {
        return mBindsState[id].second;
    }

    void InputManager::loadKeyDefaults()
    {
        // using hardcoded key defaults is inevitable, if we want the configuration files to stay valid
        // across different versions of OpenMW(in the case where another input action is added)
    	mBinds.clear();
        //Gets the Keyvalue from the Scancode; gives the button in the same place reguardless of keyboard format
        mBinds[A_Activate] = SDL_SCANCODE_SPACE;
        mBinds[A_MoveBackward] = SDL_SCANCODE_S;
        mBinds[A_MoveForward] = SDL_SCANCODE_W;
        mBinds[A_MoveLeft] = SDL_SCANCODE_A;
        mBinds[A_MoveRight] = SDL_SCANCODE_D;
        mBinds[A_ToggleWeapon] = SDL_SCANCODE_F;
        mBinds[A_ToggleSpell] = SDL_SCANCODE_R;
        mBinds[A_CycleSpellLeft] = SDL_SCANCODE_MINUS;
        mBinds[A_CycleSpellRight] = SDL_SCANCODE_EQUALS;
        mBinds[A_CycleWeaponLeft] = SDL_SCANCODE_LEFTBRACKET;
        mBinds[A_CycleWeaponRight] = SDL_SCANCODE_RIGHTBRACKET;

        mBinds[A_QuickKeysMenu] = SDL_SCANCODE_F1;
        mBinds[A_Console] = SDL_SCANCODE_GRAVE;
        mBinds[A_Run] = SDL_SCANCODE_LSHIFT;
        mBinds[A_Sneak] = SDL_SCANCODE_LCTRL;
        mBinds[A_AutoMove] = SDL_SCANCODE_Q;
        mBinds[A_Jump] = SDL_SCANCODE_E;
        mBinds[A_Journal] = SDL_SCANCODE_J;
        mBinds[A_Rest] = SDL_SCANCODE_T;
        mBinds[A_GameMenu] = SDL_SCANCODE_ESCAPE;
        mBinds[A_TogglePOV] = SDL_SCANCODE_TAB;
        mBinds[A_QuickKey1] = SDL_SCANCODE_1;
        mBinds[A_QuickKey2] = SDL_SCANCODE_2;
        mBinds[A_QuickKey3] = SDL_SCANCODE_3;
        mBinds[A_QuickKey4] = SDL_SCANCODE_4;
        mBinds[A_QuickKey5] = SDL_SCANCODE_5;
        mBinds[A_QuickKey6] = SDL_SCANCODE_6;
        mBinds[A_QuickKey7] = SDL_SCANCODE_7;
        mBinds[A_QuickKey8] = SDL_SCANCODE_8;
        mBinds[A_QuickKey9] = SDL_SCANCODE_9;
        mBinds[A_QuickKey10] = SDL_SCANCODE_0;
        mBinds[A_Screenshot] = SDL_SCANCODE_F12;
        mBinds[A_ToggleHUD] = SDL_SCANCODE_F11;
        mBinds[A_ToggleDebug] = SDL_SCANCODE_F10;
        mBinds[A_AlwaysRun] = SDL_SCANCODE_CAPSLOCK;
        mBinds[A_QuickSave] = SDL_SCANCODE_F5;
        mBinds[A_QuickLoad] = SDL_SCANCODE_F9;

        mBinds[A_Inventory] = (SDL_Scancode)(OMWI_MOUSECODE_BASE + SDL_BUTTON_RIGHT);
        mBinds[A_Use] = (SDL_Scancode)(OMWI_MOUSECODE_BASE + SDL_BUTTON_LEFT);
    }

    std::string InputManager::getActionDescription(int action)
    {
        std::map<int, std::string> descriptions;

        if (action == A_Screenshot)
            return "Screenshot";

        descriptions[A_Use] = "sUse";
        descriptions[A_Activate] = "sActivate";
        descriptions[A_MoveBackward] = "sBack";
        descriptions[A_MoveForward] = "sForward";
        descriptions[A_MoveLeft] = "sLeft";
        descriptions[A_MoveRight] = "sRight";
        descriptions[A_ToggleWeapon] = "sReady_Weapon";
        descriptions[A_ToggleSpell] = "sReady_Magic";
        descriptions[A_CycleSpellLeft] = "sPrevSpell";
        descriptions[A_CycleSpellRight] = "sNextSpell";
        descriptions[A_CycleWeaponLeft] = "sPrevWeapon";
        descriptions[A_CycleWeaponRight] = "sNextWeapon";
        descriptions[A_Console] = "sConsoleTitle";
        descriptions[A_Run] = "sRun";
        descriptions[A_Sneak] = "sCrouch_Sneak";
        descriptions[A_AutoMove] = "sAuto_Run";
        descriptions[A_Jump] = "sJump";
        descriptions[A_Journal] = "sJournal";
        descriptions[A_Rest] = "sRestKey";
        descriptions[A_Inventory] = "sInventory";
        descriptions[A_TogglePOV] = "sTogglePOVCmd";
        descriptions[A_QuickKeysMenu] = "sQuickMenu";
        descriptions[A_QuickKey1] = "sQuick1Cmd";
        descriptions[A_QuickKey2] = "sQuick2Cmd";
        descriptions[A_QuickKey3] = "sQuick3Cmd";
        descriptions[A_QuickKey4] = "sQuick4Cmd";
        descriptions[A_QuickKey5] = "sQuick5Cmd";
        descriptions[A_QuickKey6] = "sQuick6Cmd";
        descriptions[A_QuickKey7] = "sQuick7Cmd";
        descriptions[A_QuickKey8] = "sQuick8Cmd";
        descriptions[A_QuickKey9] = "sQuick9Cmd";
        descriptions[A_QuickKey10] = "sQuick10Cmd";
        descriptions[A_AlwaysRun] = "sAlways_Run";
        descriptions[A_QuickSave] = "sQuickSaveCmd";
        descriptions[A_QuickLoad] = "sQuickLoadCmd";

        if (descriptions[action] == "")
            return ""; // not configurable

        return "#{" + descriptions[action] + "}";
    }

    std::string InputManager::getActionKeyBindingName(int action)
    {
        SDL_Scancode key = mBinds[action];

        if (key == SDL_SCANCODE_UNKNOWN)
            return "#{sNone}";

        else if (key < OMWI_MOUSECODE_BASE) {
            SDL_Keycode code = SDL_GetKeyFromScancode(key);
            std::string cname = (code == SDLK_UNKNOWN)? SDL_GetScancodeName(key) : SDL_GetKeyName(code);
            return MyGUI::TextIterator::toTagsString(cname);

        } else
            return "#{sMouse} " + std::to_string(int(key) - OMWI_MOUSECODE_BASE);
    }

    std::vector<int> InputManager::getActionKeySorting()
    {
        std::vector<int> ret;
        ret.push_back(A_MoveForward);
        ret.push_back(A_MoveBackward);
        ret.push_back(A_MoveLeft);
        ret.push_back(A_MoveRight);
        ret.push_back(A_TogglePOV);
        ret.push_back(A_Run);
        ret.push_back(A_AlwaysRun);
        ret.push_back(A_Sneak);
        ret.push_back(A_Activate);
        ret.push_back(A_Use);
        ret.push_back(A_ToggleWeapon);
        ret.push_back(A_ToggleSpell);
        ret.push_back(A_CycleSpellLeft);
        ret.push_back(A_CycleSpellRight);
        ret.push_back(A_CycleWeaponLeft);
        ret.push_back(A_CycleWeaponRight);
        ret.push_back(A_AutoMove);
        ret.push_back(A_Jump);
        ret.push_back(A_Inventory);
        ret.push_back(A_Journal);
        ret.push_back(A_Rest);
        ret.push_back(A_Console);
        ret.push_back(A_QuickSave);
        ret.push_back(A_QuickLoad);
        ret.push_back(A_Screenshot);
        ret.push_back(A_QuickKeysMenu);
        ret.push_back(A_QuickKey1);
        ret.push_back(A_QuickKey2);
        ret.push_back(A_QuickKey3);
        ret.push_back(A_QuickKey4);
        ret.push_back(A_QuickKey5);
        ret.push_back(A_QuickKey6);
        ret.push_back(A_QuickKey7);
        ret.push_back(A_QuickKey8);
        ret.push_back(A_QuickKey9);
        ret.push_back(A_QuickKey10);

        return ret;
    }

    void InputManager::enableDetectingBindingMode(int action)
    {
        mNowDetecting = true;
        mDetectingAction = action;
    }

    void InputManager::keyBindingDetected(SDL_Scancode key)
    {
        //Disallow binding escape key
        if (key != SDL_SCANCODE_ESCAPE)
        {
            mBinds[mDetectingAction] = key;
        }

        mNowDetecting = false;
        MWBase::Environment::get().getWindowManager()->notifyInputActionBound();
    }

    void InputManager::mouseButtonBindingDetected(unsigned int button)
    {
        mBinds[mDetectingAction] = (SDL_Scancode)(OMWI_MOUSECODE_BASE + button);

        mNowDetecting = false;
        MWBase::Environment::get().getWindowManager()->notifyInputActionBound();
    }

    int InputManager::countSavedGameRecords() const
    {
        return 1;
    }

    void InputManager::write(ESM::ESMWriter& writer, Loading::Listener& /*progress*/)
    {
        ESM::ControlsState controls;
        controls.mViewSwitchDisabled = !getControlSwitch("playerviewswitch");
        controls.mControlsDisabled = !getControlSwitch("playercontrols");
        controls.mJumpingDisabled = !getControlSwitch("playerjumping");
        controls.mLookingDisabled = !getControlSwitch("playerlooking");
        controls.mVanityModeDisabled = !getControlSwitch("vanitymode");
        controls.mWeaponDrawingDisabled = !getControlSwitch("playerfighting");
        controls.mSpellDrawingDisabled = !getControlSwitch("playermagic");

        writer.startRecord(ESM::REC_INPU);
        controls.save(writer);
        writer.endRecord(ESM::REC_INPU);
    }

    void InputManager::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_INPU)
        {
            ESM::ControlsState controls;
            controls.load(reader);

            toggleControlSwitch("playerviewswitch", !controls.mViewSwitchDisabled);
            toggleControlSwitch("playercontrols", !controls.mControlsDisabled);
            toggleControlSwitch("playerjumping", !controls.mJumpingDisabled);
            toggleControlSwitch("playerlooking", !controls.mLookingDisabled);
            toggleControlSwitch("vanitymode", !controls.mVanityModeDisabled);
            toggleControlSwitch("playerfighting", !controls.mWeaponDrawingDisabled);
            toggleControlSwitch("playermagic", !controls.mSpellDrawingDisabled);
        }
    }

    void InputManager::resetToDefaultKeyBindings()
    {
        loadKeyDefaults();
    }

    MyGUI::MouseButton InputManager::sdlButtonToMyGUI(Uint8 button)
    {
        //The right button is the second button, according to MyGUI
        if (button == SDL_BUTTON_RIGHT)
            button = SDL_BUTTON_MIDDLE;
        else if (button == SDL_BUTTON_MIDDLE)
            button = SDL_BUTTON_RIGHT;

        //MyGUI's buttons are 0 indexed
        return MyGUI::MouseButton::Enum(button - 1);
    }

    void InputManager::setEventSinks(wrapperEventSinkType* to)
    {
        if (!to) {
            mEventSinkEnabled = false;
            return;
        }
        mEventSinks = *to;
        mEventSinkEnabled = true;
    }
}
