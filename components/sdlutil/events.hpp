#ifndef _SFO_EVENTS_H
#define _SFO_EVENTS_H

#include <SDL_types.h>
#include <SDL_events.h>

////////////
// Events //
////////////

namespace SDLUtil
{

/** Extended mouse event struct where we treat the wheel like an axis, like everyone expects */
struct MouseMotionEvent : SDL_MouseMotionEvent {

    Sint32 zrel;
    Sint32 z;
};


///////////////
// Listeners //
///////////////

class MouseListener
{
public:
    virtual ~MouseListener() {}
    virtual void mouseMoved( const MouseMotionEvent &arg ) = 0;
    virtual void mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id ) = 0;
    virtual void mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id ) = 0;
};

class KeyListener
{
public:
    virtual ~KeyListener() {}
    virtual void textInput (const SDL_TextInputEvent& arg) {}
    virtual void keyPressed(const SDL_KeyboardEvent &arg) = 0;
    virtual void keyReleased(const SDL_KeyboardEvent &arg) = 0;
};

class WindowListener
{
public:
    virtual ~WindowListener() {}

    /** @remarks The window's visibility changed */
    virtual void windowVisibilityChange( bool visible ) {}

    /** @remarks The window got / lost input focus */
    virtual void windowFocusChange( bool have_focus ) {}

    virtual void windowClosed () {}

    virtual void windowResized (int x, int y) {}
};

}

#endif
