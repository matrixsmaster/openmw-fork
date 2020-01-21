#include "vmo_generator.hpp"

#include <cstdio>

#include <extern/xswrapper/xswrapper.hpp>

namespace MWSound
{

VMO_Generator::VMO_Generator() :
    Sound_Decoder(nullptr)
{
    printf("VMO Sound Generator Created\n");
}

VMO_Generator::~VMO_Generator()
{
    printf("VMO Sound Generator Destroyed\n");
}

void VMO_Generator::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{
    //FIXME: add proper wait for a first LDB sound info event
#if 0
    dosbox::LDB_SoundInfo* ptr = wrapperGetSoundConfig();
    if (!ptr) return;

    *samplerate = ptr->freq;
    *chans = (ptr->channels == 1)? ChannelConfig_Mono : ChannelConfig_Stereo;

    if (ptr->width == 2 && ptr->sign) *type = SampleType_Int16;
    else if (ptr->width == 1 && !ptr->sign) *type = SampleType_UInt8;

    printf("VMO gen getInfo() called: %d, %d, %d\n",*samplerate,*chans,*type);
#else
    // I'm so tired right now, I'll just leave it here
    *samplerate = 44100;
    *chans = ChannelConfig_Stereo;
    *type = SampleType_Int16;
#endif
}

size_t VMO_Generator::read(char *buffer, size_t bytes)
{
    int r = wrapperGetSound((uint8_t*)buffer,bytes);
    //printf("read(): %d\n",r);
    return r;
}

}
