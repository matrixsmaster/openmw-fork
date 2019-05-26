/*
 *  Copyright (C) 2013-2019  Dmitry Soloviov
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

#include <vector>
#include "xswrapper.hpp"
#include "xskbd.hpp"
#include "soundr.hpp"
#include "dosbox.h"

/*
 * The Basic model:
 * 1. Run SDL window
 * 2. Start Box thread
 * 2.a. Box thread calls UpdateScreenBuffer() every it's own frame
 * 2.b. Box thread calls UpdateSoundBuffer() every time the buffer is ready
 * 2.c. Box thread calls QueryUIEvents() every time the thread ready to process them
 * 2.note. This mechanism is close enough to what will happen on a real chip
 * (video generator may be a separate chip, which connected to main chip via UART/SPI/I2C
 * or may be just a timer's interrupt handler; audio output controller may be implemented
 * as simple timer interrupt, which updates the other timer's values to change duty
 * cycle of PWM generated; keyboard handler is a pin change interrupt handler too;
 * all of this parts will never directly affect the main code)
 *
 * Excessive debug output needed for future stress tests and profiling. So better I'll
 * write it now :)
 */

static dosbox::CDosBox* doscard = NULL;
static SDL_Thread* dosboxthr = NULL;
static SDL_Window* wnd = NULL;
static SDL_Renderer* ren = NULL;
static struct timespec* clkres = NULL;
static uint32_t* clkbeg = NULL;
static uint8_t disp_fsm = 0;
static uint16_t lcd_w,lcd_h;
static uint32_t frame_cnt;
static uint32_t* framebuf = NULL;
static SDL_Texture* frame_sdl = NULL;
static bool frame_dirty = false;
static std::vector<dosbox::LDB_UIEvent> evt_fifo;
static int frame_block = 0;
SDL_atomic_t at_flag;
static XS_SoundRing sndring;
static SDL_AudioDeviceID audio = 0;
static uint8_t frameskip_cnt;

#define FRAMESKIP_MAX 10

using namespace std;
using namespace dosbox;

int32_t XS_UpdateScreenBuffer(void* buf, size_t len)
{
    if (!buf) return -1;
    uint32_t* dw;
    uint8_t* b;

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
            if (disp_fsm) {
                disp_fsm = 1;
                if (SDL_AtomicGet(&at_flag) >= 0)
                    SDL_AtomicIncRef(&at_flag);
            }
            break;

        default:
            if (disp_fsm == 1) {
                if (SDL_AtomicGet(&at_flag) > 0) {
                    if (frameskip_cnt++ >= FRAMESKIP_MAX) {
                        while (SDL_AtomicGet(&at_flag) > 0) ;
                    } else
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
            if (SDL_AtomicGet(&at_flag) >= 0)
                SDL_AtomicIncRef(&at_flag);
            disp_fsm = 1;
        }

    } else
        return -1;

    return 0;
}

int32_t XS_UpdateSoundBuffer(void* buf, size_t len)
{
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

        audio = SDL_OpenAudioDevice(NULL,0,&want,&have,SDL_AUDIO_ALLOW_FORMAT_CHANGE);
        if (!audio) return 0;

        memset(sndring.data,0,sizeof(sndring.data));
        sndring.read = 0;
        sndring.write = 0;
        sndring.paused = true;
        SDL_PauseAudioDevice(audio,1);
        return 0;
    }
    if (!audio) return 0;

    SDL_LockAudioDevice(audio);

    len >>= 1;
    int16_t* in = reinterpret_cast<int16_t*>(buf);
    int p = sndring.write;

    for (i=0; i<len; i++) {
        sndring.data[p] = in[i];
        if (++p >= XSHELL_SOUND_LENGTH) p = 0;
    }
    sndring.write = p;

    SDL_UnlockAudioDevice(audio);

    if (sndring.paused) {
        sndring.paused = false;
        SDL_PauseAudioDevice(audio,0);
    }
    return 0;
}

int32_t XS_QueryUIEvents(void* buf, size_t len)
{
    if ((!buf) || (len < sizeof(LDB_UIEvent))) return -1;
    if (SDL_AtomicGet(&at_flag) > 0) return 0;

    int r = evt_fifo.size();
    if (!evt_fifo.empty()) {
        LDB_UIEvent e = evt_fifo.back();
        evt_fifo.pop_back();
        memcpy(buf,&e,sizeof(LDB_UIEvent));
    }

    if (SDL_AtomicGet(&at_flag) >= 0)
        SDL_AtomicIncRef(&at_flag);

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
    doscard->RegisterCallback(DBCB_PushMessage,&XS_Message);
    doscard->RegisterCallback(DBCB_FileIOReq,&XS_FIO);
    doscard->RegisterCallback(DBCB_LogSTDOUT,&XS_Message); //to test it out :)
}

static int XS_SDLInit()
{
    lcd_w = XSHELL_DEF_WND_W;
    lcd_h = XSHELL_DEF_WND_H;
    SDL_AtomicSet(&at_flag,0);
    return 0;
}

static void XS_SDLKill()
{
}

static void XS_SDLoop()
{
    SDL_Event e;
    LDB_UIEvent mye;
    uint32_t i;
    bool quit = false;

    do {
        while (!SDL_AtomicGet(&at_flag))
            if (!doscard) return;           //DOS thread had finished

        /* Event Processing*/
        while (SDL_PollEvent(&e)) {

            memset(&mye,0,sizeof(mye));
            switch (e.type) {

            case SDL_QUIT:
                mye.t = LDB_UIE_QUIT;
                quit = true;
                break;

            case SDL_KEYDOWN:
                if (e.key.keysym.scancode == SDL_SCANCODE_PAUSE) {
                    frame_block ^= 1;
                    continue;
                }
                //no break

            case SDL_KEYUP:
                mye.t = LDB_UIE_KBD;
                mye.pressed = (e.type == SDL_KEYDOWN);
                mye.key = KBD_NONE;
                //FIXME: use key-list rather than this brute-force!
                for (i=0; i<(sizeof(XShellKeyboardMap)/sizeof(XShellKeyboardPair)); i++)
                    if (XShellKeyboardMap[i].sdl == e.key.keysym.scancode) {
                        mye.key = XShellKeyboardMap[i].db;
                        break;
                    }
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                mye.pressed = (e.button.state == SDL_PRESSED);
                switch (e.button.button) {
                case SDL_BUTTON_LEFT:   mye.m.button = 1; break;
                case SDL_BUTTON_RIGHT:  mye.m.button = 2; break;
                default:                mye.m.button = 3; break;
                }
                //no break

            case SDL_MOUSEMOTION:
                mye.t = LDB_UIE_MOUSE;
                //FIXME: mouse moves looks not so good, investigation needed
                mye.m.rel.x = static_cast<float>(e.motion.xrel);
                mye.m.rel.y = static_cast<float>(e.motion.yrel);
                mye.m.abs.x = static_cast<float>(e.motion.x);
                mye.m.abs.y = static_cast<float>(e.motion.y);
                break;

            default: continue;
            }

            evt_fifo.insert(evt_fifo.begin(),mye);
        }

        /* Frame Processing*/
        if ((!frame_block) && (framebuf)) {
            if (frame_dirty) {
                if (frame_sdl) SDL_DestroyTexture(frame_sdl);
                frame_sdl = SDL_CreateTexture(ren,SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING,lcd_w,lcd_h);
                frame_dirty = false;
            }
            if (frame_sdl) {
                SDL_UpdateTexture(frame_sdl,NULL,framebuf,lcd_w*sizeof(uint32_t));
                SDL_RenderClear(ren);
                SDL_RenderCopy(ren,frame_sdl,NULL,NULL);
            }
        }
        SDL_AtomicSet(&at_flag,0);

        /* Update Window*/
        SDL_RenderPresent(ren);
        SDL_Delay(5);           //FIXME: MAGIC delay
    } while (!quit);
    SDL_AtomicSet(&at_flag,-10);
}

int32_t XS_Message(void* buf, size_t len)
{
    return 0;
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
    doscard->Execute();
    delete doscard;
    doscard = NULL;
    return 0;
}
#if 0
int main(int argc, char* argv[])
{
    int r;
    xnfo(0,1,"ALIVE!");

    // Create DOSCard class instance
    doscard = new CDosBox();
    if (doscard) xnfo(0,1,"Class instance created");
    else abort();

    // Create SDL2 Window
    if (XS_SDLInit()) xnfo(-1,1,"Unable to create SDL2 context!");
    xnfo(0,1,"SDL2 context created successfully");

    // Register our callbacks to doscard core
    XS_ldb_register();

    // Request for 64MB RAM
    LDB_Settings* setts = doscard->GetConfig();
    setts->mem.total_ram = 64;
    doscard->SetConfig(setts);

    // Create SDL2 Thread for DOS and run it
    dosboxthr = SDL_CreateThread(DosRun,"DosThread",NULL);
    if (!dosboxthr) xnfo(-1,1,"Unable to create DOS thread!");
    xnfo(0,1,"DOS Thread running!");

    // Just a loop entry point :)
    XS_SDLoop();
    xnfo(0,1,"SDLoop() Exited");

    // Now, when our loop is finished for some reason, just wait for thread
    SDL_WaitThread(dosboxthr,&r);
    xnfo(0,1,"DOS Thread Exited (%d)",r);

    // ...and close the window
    XS_SDLKill();

    // We're done!
    xnfo(0,1,"QUIT");
    return (EXIT_SUCCESS);
}
#endif
