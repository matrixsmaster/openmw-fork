/*
 *  Copyright (C) 2013-2019  Dmitry Solovyev
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

/*
 * ATTENTION! WARNING! ACHTUNG!
 * This module IS NOT FINISHED, it's just a roughly changed copy of DosCard demo XShell 1
 * Made for proof-of-concept purposes
 *
 * TODO: fix all this mess
 */

#include <vector>
#include <pthread.h>
#include <SDL2/SDL.h>
#include "xswrapper.hpp"
#include "xskbd.hpp"
#include "soundr.hpp"
#include "dosbox.h"
#include "cpu.h"

static dosbox::CDosBox* doscard = NULL;
static SDL_Thread* dosboxthr = NULL;
static uint8_t disp_fsm = 0;
static uint16_t lcd_w,lcd_h;
static uint32_t frame_cnt;
static uint32_t* framebuf = NULL;
static bool frame_dirty = false;
static std::vector<dosbox::LDB_UIEvent> evt_fifo;
static pthread_mutex_t update_mutex;
static XS_SoundRing sndring;
static uint8_t frameskip_cnt;
static wrapperEventSinkType cur_sinks;
static bool rctrl_down;
static bool outtex_valid = false;
osg::ref_ptr<osg::Texture2D> outtexture;

#define FRAMESKIP_MAX 10

using namespace std;
using namespace dosbox;

int32_t XS_UpdateScreenBuffer(void* buf, size_t len)
{
    if (!buf) return -1;
    uint32_t* dw;

    pthread_mutex_lock(&update_mutex);

    if (len == 4) {
        dw = reinterpret_cast<uint32_t*>(buf);
        switch (*dw) {
        case DISPLAY_INIT_SIGNATURE:
            frameskip_cnt = 0;
            if (!disp_fsm) disp_fsm = 1;
            break;

        case DISPLAY_NFRM_SIGNATURE:
            disp_fsm = 1;
            break;

        case DISPLAY_ABOR_SIGNATURE:
            if (disp_fsm) disp_fsm = 1;
            break;

        default:
            if (disp_fsm == 1) {
                if (frameskip_cnt++ < FRAMESKIP_MAX) {
                    pthread_mutex_unlock(&update_mutex);
                    return DISPLAY_RET_BUSY;
                }
                frameskip_cnt = 0;

                uint16_t old_w = lcd_w;
                uint16_t old_h = lcd_h;
                lcd_w = (*dw >> 16);
                lcd_h = *dw & 0xffff;
                frame_cnt = 0;
                disp_fsm = 2;
                if ((!framebuf) || (old_w*old_h != lcd_w*lcd_h)) {
                    framebuf = reinterpret_cast<uint32_t*>(realloc(framebuf,sizeof(uint32_t)*lcd_w*lcd_h));
                    frame_dirty = true;
                }
            }
            break;
        }

    } else if ((disp_fsm == 2) && (len == lcd_w * 4)) {
        memcpy(framebuf+(lcd_w*frame_cnt),buf,len);
        if (++frame_cnt >= lcd_h) {
            disp_fsm = 1;
        }
    }

    pthread_mutex_unlock(&update_mutex);

    return 0;
}

int32_t XS_UpdateSoundBuffer(void* buf, size_t len)
{
#if 1
    int i;
    if (!buf) return -1;

    if (len == sizeof(LDB_SoundInfo)) {
        SDL_AudioSpec want,have;
        LDB_SoundInfo* req = reinterpret_cast<LDB_SoundInfo*>(buf);
        SDL_zero(want);
        if (req->silent) return 0;

        want.freq = req->freq;
        if (req->sign && (req->width == 16)) want.format = AUDIO_S16SYS;
        want.channels = req->channels;
        want.samples = req->blocksize;
        want.callback = XS_AudioCallback;

//        audio = SDL_OpenAudioDevice(NULL,0,&want,&have,SDL_AUDIO_ALLOW_FORMAT_CHANGE);
//        if (!audio) return 0;
//
//        memset(sndring.data,0,sizeof(sndring.data));
//        sndring.read = 0;
//        sndring.write = 0;
//        sndring.paused = true;
//        SDL_PauseAudioDevice(audio,1);
        return 0;
    }
//    if (!audio) return 0;
//
//    SDL_LockAudioDevice(audio);
//
//    len >>= 1;
//    int16_t* in = reinterpret_cast<int16_t*>(buf);
//    int p = sndring.write;
//
//    for (i=0; i<len; i++) {
//        sndring.data[p] = in[i];
//        if (++p >= XSHELL_SOUND_LENGTH) p = 0;
//    }
//    sndring.write = p;
//
//    SDL_UnlockAudioDevice(audio);
//
//    if (sndring.paused) {
//        sndring.paused = false;
//        SDL_PauseAudioDevice(audio,0);
//    }
#endif
    return 0;
}

int32_t XS_QueryUIEvents(void* buf, size_t len)
{
    if ((!buf) || (len < sizeof(LDB_UIEvent))) return -1;

    pthread_mutex_lock(&update_mutex);

    int r = evt_fifo.size();

    if (!evt_fifo.empty()) {
        LDB_UIEvent e = evt_fifo.back();
        evt_fifo.pop_back();
        memcpy(buf,&e,sizeof(LDB_UIEvent));
    }

    pthread_mutex_unlock(&update_mutex);

    return r;
}

int32_t XS_GetTicks(void* buf, size_t len)
{
    if ((!buf) || (len < 4)) return -1;
    uint32_t* val = reinterpret_cast<uint32_t*>(buf);
    *val = SDL_GetTicks();
    return 0;
}

static void XS_ldb_register()
{
    doscard->RegisterCallback(DBCB_GetTicks,&XS_GetTicks);
    doscard->RegisterCallback(DBCB_PushScreen,&XS_UpdateScreenBuffer);
    doscard->RegisterCallback(DBCB_PushSound,&XS_UpdateSoundBuffer);
    doscard->RegisterCallback(DBCB_PullUIEvents,&XS_QueryUIEvents);
    doscard->RegisterCallback(DBCB_FileIOReq,&XS_FIO);
}

static void XS_SDLInit()
{
    lcd_w = XSHELL_DEF_WND_W;
    lcd_h = XSHELL_DEF_WND_H;
    rctrl_down = false;

    pthread_mutex_init(&update_mutex,NULL);
}

static void XS_SDLKill()
{
    int r;

    if (doscard) doscard->SetQuit();

    if (dosboxthr) {
        SDL_WaitThread(dosboxthr,&r);
        dosboxthr = NULL;
    }

    if (doscard) {
        delete doscard;
        doscard = NULL;
    }

    pthread_mutex_destroy(&update_mutex);
}

int32_t XS_FIO(void* buf, size_t len)
{
    uint64_t* x;
    int64_t* sx;
    struct stat tstat;
    struct dirent* pdir;

    if ((!buf) || (len < sizeof(DBFILE))) return -1;
    DBFILE* f = reinterpret_cast<DBFILE*>(buf);

    if ((f->todo > 0) && (f->todo < 10) && (!f->rf)) return -1;
    if ((f->todo > 20) && (!f->df)) return -1;

    switch (f->todo) {
    case 0:
        //open
        //TODO: track opened files
        f->rf = fopen(f->name,f->op);
        if (!f->rf) return -1;
        break;

    case 1:
        //close
        fclose(f->rf);
        break;

    case 2:
        //fread
        return (fread(f->buf,f->p_x,f->p_y,f->rf));

    case 3:
        //fwrite
        return (fwrite(f->buf,f->p_x,f->p_y,f->rf));

    case 4:
        //fseek
        if (f->p_y != 8) return -1;
        x = reinterpret_cast<uint64_t*>(f->buf);
        return (fseek(f->rf,*x,f->p_x));

    case 5:
        //ftell
        if (f->p_y != 8) return 0;
        x = reinterpret_cast<uint64_t*>(f->buf);
        *x = ftell(f->rf);
        break;

    case 6:
        //feof
        return (feof(f->rf));

    case 7:
        //ftruncate
        if (f->p_y != 8) return -1;
        sx = reinterpret_cast<int64_t*>(f->buf);
        return (ftruncate(fileno(f->rf),*sx));

    case 10:
        //isfileexist
        if ((stat(f->name,&tstat) == 0) && (tstat.st_mode & S_IFREG)) return 0;
        return -1;

    case 11:
        //isdirexist
        if ((stat(f->name,&tstat) == 0) && (tstat.st_mode & S_IFDIR)) return 0;
        return -1;

    case 12:
        //get file size (we can't handle >2GB filesize, but that's OK)
        if (!stat(f->name,&tstat)) return static_cast<int>(tstat.st_size);
        break;

    case 20:
        //open dir
        f->df = opendir(f->name);
        if (!f->df) return -1;
        break;

    case 21:
        //read dir (X param = is directory)
        pdir = readdir(f->df);
        if (!pdir) return -1;
        strncpy(reinterpret_cast<char*>(f->buf),pdir->d_name,f->p_y);
        f->p_x = (pdir->d_type == DT_DIR)? 1:0;
        break;

    case 22:
        //close dir
        closedir(f->df);
        break;

    default:
        return -1;
    }

    return 0;
}

void XS_AudioCallback(void* userdata, uint8_t* stream, int len)
{
    int i,p;
    int16_t* buf = reinterpret_cast<int16_t*>(stream);
    len >>= 1;
    //FIXME: better use memcpy()
    p = sndring.read;
    for (i=0; i<len; i++) {
        buf[i] = sndring.data[p];
        if (++p >= XSHELL_SOUND_LENGTH) p = 0;
    }
    sndring.read = p;
}

int DosRun(void* p)
{
    doscard->UnlockSpeed(true);
    doscard->Execute();
    delete doscard;
    doscard = NULL;
    return 0;
}

int wrapperInit()
{
    // Check previous instance existence
    if (doscard || dosboxthr) XS_SDLKill();

    // Create DOSCard class instance
    XS_SDLInit();
    doscard = new CDosBox();
    if (!doscard) return -1;

    // Register our callbacks to doscard core
    XS_ldb_register();

    // Request for 64MB RAM
    LDB_Settings* setts = doscard->GetConfig();
    setts->mem.total_ram = 64;
    setts->cpu.core = ALDB_CPU::LDB_CPU_NORMAL;
//    setts->cpu.cycle_limit = ALDB_CPU::LDB_CPU_CYCLE_MAX;
    setts->cpu.family = CPU_ARCHTYPE_386FAST;
    setts->frameskip = 0;
    doscard->SetConfig(setts);

    // Create SDL2 Thread for DOS and run it
    dosboxthr = SDL_CreateThread(DosRun,"DosThread",NULL);
    if (!dosboxthr) return -1;

    return 0;
}

int wrapperKill()
{
    XS_SDLKill();
    return 0;
}

static void* revmemcpy(void* dest, void* src, size_t len, size_t unit)
{
    if (!dest || !src ) return NULL;
    if (!len || !unit) return dest;
    if (len % unit) return dest;

    uint8_t* _dest = (uint8_t*)dest;
    uint8_t* _src = (uint8_t*)src;

    _dest += len - unit;
    for (size_t i = 0; i < len; i += unit, _dest -= unit, _src += unit)
        memcpy(_dest,_src,unit);

    return dest;
}

osg::ref_ptr<osg::Texture2D> wrapperGetFrame()
{
#if 0
    osg::ref_ptr<osg::Texture2D> txd(new osg::Texture2D());

    if (!doscard || !framebuf) {
        printf("Unable to render VM frame\n");
        return txd;
    }

    osg::ref_ptr<osg::Image> img(new osg::Image());

    pthread_mutex_lock(&update_mutex);

    img.get()->allocateImage(lcd_w,lcd_h,1,GL_RGBA,GL_UNSIGNED_BYTE);

//    memcpy(img.get()->data(),framebuf,lcd_w*lcd_h*4);
    revmemcpy(img.get()->data(),framebuf,lcd_w*lcd_h*4,lcd_w*4);

    pthread_mutex_unlock(&update_mutex);

    txd->setImage(img);

    return txd;
#else
    if (!outtex_valid) {
        outtexture = osg::ref_ptr<osg::Texture2D>(new osg::Texture2D());
        outtexture.get()->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
        outtexture.get()->setFilter(osg::Texture::MAG_FILTER,osg::Texture::LINEAR);
        outtexture.get()->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        outtexture.get()->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        outtexture.get()->setDataVariance(osg::Object::DYNAMIC);

        osg::ref_ptr<osg::Image> img(new osg::Image());

        pthread_mutex_lock(&update_mutex);
        img.get()->allocateImage(lcd_w,lcd_h,1,GL_RGBA,GL_UNSIGNED_BYTE);
        pthread_mutex_unlock(&update_mutex);

        outtexture->setImage(img);

        outtex_valid = true;
    }

    if (!doscard || !framebuf) return outtexture;

    pthread_mutex_lock(&update_mutex);

    if (frame_dirty) {
        outtex_valid = false;
        frame_dirty = false;
        pthread_mutex_unlock(&update_mutex);
        return outtexture;
    }

    if (outtexture.get()->getImage()) {
        revmemcpy(outtexture.get()->getImage()->data(),framebuf,lcd_w*lcd_h*4,lcd_w*4);
        outtexture.get()->getImage()->dirty();
    }

    pthread_mutex_unlock(&update_mutex);

    return outtexture;
#endif
}

static void insertEvent(LDB_UIEventE t, KBD_KEYS key, bool pressed, LDB_MOUSEINF* mouse)
{
    LDB_UIEvent ev;
    memset(&ev,0,sizeof(ev));

    ev.t = t;
    ev.key = key;
    ev.pressed = pressed;
    if (mouse) ev.m = *mouse;

    pthread_mutex_lock(&update_mutex);
    evt_fifo.insert(evt_fifo.begin(),ev);
    pthread_mutex_unlock(&update_mutex);

//    printf("Wrapper: event added: %d, %d, %d, %p\n", t, key, pressed, mouse);
}

static KBD_KEYS mapSdl2Kbd(SDL_Scancode code)
{
    for (unsigned i = 0; i < (sizeof(XShellKeyboardMap)/sizeof(XShellKeyboardPair)); i++) {
        if (XShellKeyboardMap[i].sdl == code)
            return XShellKeyboardMap[i].db;
    }
    return KBD_NONE;
}

static bool wrapperKeyDownEvent(const SDL_KeyboardEvent* ev)
{
    if (!ev) return true;

    if (rctrl_down && ev->keysym.scancode == SDL_SCANCODE_HOME) {
        rctrl_down = false;
        return false; //exit
    }

    if (ev->keysym.scancode == SDL_SCANCODE_RCTRL) rctrl_down = true;

    insertEvent(LDB_UIE_KBD, mapSdl2Kbd(ev->keysym.scancode), true, NULL);
    return true;
}

static bool wrapperKeyUpEvent(const SDL_KeyboardEvent* ev)
{
    if (!ev) return true;

    if (ev->keysym.scancode == SDL_SCANCODE_RCTRL) rctrl_down = false;

    insertEvent(LDB_UIE_KBD, mapSdl2Kbd(ev->keysym.scancode), false, NULL);
    return true;
}

static LDB_MOUSEINF mapSdl2MouseClick(uint8_t button)
{
    LDB_MOUSEINF inf;
    memset(&inf,0,sizeof(inf));
    inf.button = button;
    return inf;
}

static bool wrapperMouseDownEvent(const SDL_MouseButtonEvent* ev)
{
    if (!ev) return true;

    LDB_MOUSEINF tmp = mapSdl2MouseClick(ev->button);
    insertEvent(LDB_UIE_MOUSE, KBD_NONE, true, &tmp);
    return true;
}


static bool wrapperMouseUpEvent(const SDL_MouseButtonEvent* ev)
{
    if (!ev) return true;

    LDB_MOUSEINF tmp = mapSdl2MouseClick(ev->button);
    insertEvent(LDB_UIE_MOUSE, KBD_NONE, false, &tmp);
    return true;
}

static bool wrapperMouseEvent(const SDL_MouseMotionEvent* ev)
{
    if (!ev) return true;

    LDB_MOUSEINF inf;
    memset(&inf,0,sizeof(inf));
    inf.rel.x = static_cast<float>(ev->xrel);
    inf.rel.y = static_cast<float>(ev->yrel);
    inf.abs.x = static_cast<float>(ev->x);
    inf.abs.y = static_cast<float>(ev->y);

    insertEvent(LDB_UIE_MOUSE, KBD_NONE, false, &inf);
    return true;
}

wrapperEventSinkType* wrapperGetEventSinks()
{
    cur_sinks.keydown = wrapperKeyDownEvent;
    cur_sinks.keyup = wrapperKeyUpEvent;
    cur_sinks.mousedown = wrapperMouseDownEvent;
    cur_sinks.mouseup = wrapperMouseUpEvent;
    cur_sinks.mouse = wrapperMouseEvent;

    return &cur_sinks;
}
