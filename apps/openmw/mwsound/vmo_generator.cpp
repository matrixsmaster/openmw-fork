#include "vmo_generator.hpp"

#include <cstdio>

#include <extern/xswrapper/xswrapper.hpp>

namespace MWSound
{

VMO_Generator::VMO_Generator() :
    Sound_Decoder(nullptr)
{
}

void VMO_Generator::getInfo(int *samplerate, ChannelConfig *chans, SampleType *type)
{

}

size_t VMO_Generator::read(char *buffer, size_t bytes)
{

}

}
