#ifndef MWINPUT_MWINPUTMANAGERIMP_H
#define MWINPUT_MWINPUTMANAGERIMP_H

#include "../mwgui/mode.hpp"

#include <map>
#include <osg/ref_ptr>
#include <osgViewer/ViewerEventHandlers>

#include <SDL_version.h>

#include <components/settings/settings.hpp>
#include <components/files/configurationmanager.hpp>
#include <components/sdlutil/events.hpp>

#include "../mwbase/inputmanager.hpp"

namespace MWWorld
{
    class Player;
}

namespace MWBase
{
    class WindowManager;
}

namespace MyGUI
{
    struct MouseButton;
}

namespace Files
{
    struct ConfigurationManager;
}

namespace SDLUtil
{
    class InputWrapper;
    class VideoWrapper;
}

namespace osgViewer
{
    class Viewer;
    class ScreenCaptureHandler;
}

struct SDL_Window;

#define OMWI_MOUSECODE_BASE (SDL_NUM_SCANCODES + 1)

// TODO: remove non-existent methods

namespace MWInput
{
    /**
    * @brief Class that handles all input and key bindings for OpenMW.
    */
    class InputManager :
            public MWBase::InputManager,
            public SDLUtil::KeyListener,
            public SDLUtil::MouseListener,
            public SDLUtil::WindowListener
    {
    public:
        InputManager(
            SDL_Window* window,
            osg::ref_ptr<osgViewer::Viewer> viewer,
            osg::ref_ptr<osgViewer::ScreenCaptureHandler> screenCaptureHandler,
            osgViewer::ScreenCaptureHandler::CaptureOperation *screenCaptureOperation,
            const std::string& userFile, bool userFileExists, bool grab);

        virtual ~InputManager();

        virtual bool isWindowVisible();

        /// Clear all savegame-specific data
        virtual void clear();

        virtual void update(float dt, bool disableControls=false, bool disableEvents=false);

        void setPlayer(MWWorld::Player* player) { mPlayer = player; }

        virtual void changeInputMode(bool guiMode);

        virtual void processChangedSettings(const Settings::CategorySettingVector& changed);

        virtual void setDragDrop(bool dragDrop);

        virtual void toggleControlSwitch(const std::string& sw, bool value);
        virtual bool getControlSwitch(const std::string& sw);

        virtual std::string getActionDescription(int action);
        virtual std::string getActionKeyBindingName(int action);
        virtual int getNumActions() { return A_Last; }
        virtual std::vector<int> getActionKeySorting();
        virtual void enableDetectingBindingMode(int action);
        virtual void resetToDefaultKeyBindings();

    public:
        virtual void keyPressed(const SDL_KeyboardEvent &arg );
        virtual void keyReleased( const SDL_KeyboardEvent &arg );
        virtual void textInput(const SDL_TextInputEvent &arg);

        virtual void mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id );
        virtual void mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id );
        virtual void mouseMoved( const SDLUtil::MouseMotionEvent &arg );

        virtual void windowVisibilityChange( bool visible );
        virtual void windowFocusChange( bool have_focus );
        virtual void windowResized(int x, int y);
        virtual void windowClosed();

        virtual int countSavedGameRecords() const;
        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress);
        virtual void readRecord(ESM::ESMReader& reader, uint32_t type);

        virtual void setEventSinks(wrapperEventSinkType* to);
        virtual bool isEventSinkEnabled() const;

    private:
        SDL_Window* mWindow;
        bool mWindowVisible;
        osg::ref_ptr<osgViewer::Viewer> mViewer;
        osg::ref_ptr<osgViewer::ScreenCaptureHandler> mScreenCaptureHandler;
        osgViewer::ScreenCaptureHandler::CaptureOperation *mScreenCaptureOperation;

        MWWorld::Player* mPlayer;

        std::map<int, SDL_Scancode> mBinds;
        std::map<int, bool> mBindsEnabled;
        std::map<int, std::pair<int, int> > mBindsState;

        SDLUtil::InputWrapper* mInputManager;
        SDLUtil::VideoWrapper* mVideoWrapper;

        std::string mUserFile;

        bool mDragDrop;

        bool mGrabCursor;

        bool mInvertY;

        bool mControlsDisabled;

        float mCameraSensitivity;
        float mCameraYMultiplier;
        float mPreviewPOVDelay;
        float mTimeIdle;

        bool mMouseLookEnabled;
        bool mGuiCursorEnabled;

        bool mNowDetecting;
        int mDetectingAction;

        float mOverencumberedMessageDelay;

        float mGuiCursorX;
        float mGuiCursorY;
        int mMouseWheel;
        bool mUserFileExists;
        bool mAlwaysRunActive;
        bool mSneakToggles;
        bool mSneaking;
        bool mAttemptJump;

        std::map<std::string, bool> mControlSwitch;

        float mInvUiScalingFactor;

        bool mEventSinkEnabled;
        wrapperEventSinkType mEventSinks;

    private:
        void convertMousePosForMyGUI(int& x, int& y);

        MyGUI::MouseButton sdlButtonToMyGUI(Uint8 button);

        void resetIdleTime();
        void updateIdleTime(float dt);

        void setPlayerControlsEnabled(bool enabled);
        void handleGuiArrowKey(int action);

        void updateCursorMode();

        bool checkAllowedToUseItems() const;

        void channelChanged(int action);
        void keyBindingDetected(SDL_Scancode key);
        void mouseButtonBindingDetected(unsigned int button);

        void fireUpBind(SDL_Scancode key, int state);

    private:
        void toggleMainMenu();
        void toggleSpell();
        void toggleWeapon();
        void toggleInventory();
        void toggleConsole();
        void screenshot();
        void toggleJournal();
        void activate();
        void toggleWalking();
        void toggleSneaking();
        void toggleAutoMove();
        void rest();
        void quickLoad();
        void quickSave();

        void quickKey(int index);
        void showQuickKeysMenu();

        bool actionIsActive(int id);

        void loadKeyDefaults();

        int mFakeDeviceID; //As we only support one controller at a time, use a fake deviceID so we don't lose bindings when switching controllers

    private:
        enum Actions
        {
            // please add new actions at the bottom, in order to preserve the channel IDs in the key configuration files

            A_GameMenu,

            A_Unused,

            A_Screenshot,     // Take a screenshot

            A_Inventory,      // Toggle inventory screen

            A_Console,        // Toggle console screen

            A_MoveLeft,       // Move player left / right
            A_MoveRight,
            A_MoveForward,    // Forward / Backward
            A_MoveBackward,

            A_Activate,

            A_Use,        //Use weapon, spell, etc.
            A_Jump,
            A_AutoMove,   //Toggle Auto-move forward
            A_Rest,       //Rest
            A_Journal,    //Journal
            A_Weapon,     //Draw/Sheath weapon
            A_Spell,      //Ready/Unready Casting
            A_Run,        //Run when held
            A_CycleSpellLeft, //cycling through spells
            A_CycleSpellRight,
            A_CycleWeaponLeft,//Cycling through weapons
            A_CycleWeaponRight,
            A_ToggleSneak,    //Toggles Sneak
            A_AlwaysRun, //Toggle Walking/Running
            A_Sneak,

            A_QuickSave,
            A_QuickLoad,
            A_QuickMenu,
            A_ToggleWeapon,
            A_ToggleSpell,

            A_TogglePOV,

            A_QuickKey1,
            A_QuickKey2,
            A_QuickKey3,
            A_QuickKey4,
            A_QuickKey5,
            A_QuickKey6,
            A_QuickKey7,
            A_QuickKey8,
            A_QuickKey9,
            A_QuickKey10,

            A_QuickKeysMenu,

            A_ToggleHUD,

            A_ToggleDebug,

            A_LookUpDown,         //Joystick look
            A_LookLeftRight,
            A_MoveForwardBackward,
            A_MoveLeftRight,

            A_Last            // Marker for the last item
        };
    };
}
#endif
