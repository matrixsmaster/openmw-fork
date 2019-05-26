/*
 *  Copyright (C) 2019  Dmitry Solovyev
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef XSWRAPPER_EVENTSINK_HPP_
#define XSWRAPPER_EVENTSINK_HPP_

#include <functional>
#include <SDL2/SDL.h>

struct wrapperEventSinkType {
    std::function<bool (const SDL_KeyboardEvent*)> keydown = 0;
    std::function<bool (const SDL_KeyboardEvent*)> keyup = 0;
    std::function<bool (const SDL_MouseButtonEvent*)> mousedown = 0;
    std::function<bool (const SDL_MouseButtonEvent*)> mouseup = 0;
    std::function<bool (const SDL_MouseMotionEvent*)> mouse = 0;
};

#endif /* XSWRAPPER_EVENTSINK_HPP_ */
