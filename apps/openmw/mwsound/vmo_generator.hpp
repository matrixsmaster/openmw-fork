#ifndef MWSOUND_VMO_GENERATOR_HPP_
#define MWSOUND_VMO_GENERATOR_HPP_

#include "sound_decoder.hpp"

namespace MWSound
{

    struct VMO_Generator : public Sound_Decoder
    {
        virtual void open(const std::string &fname) {}
        virtual void close() {}

        virtual std::string getName() { return std::string(); }
        virtual void getInfo(int *samplerate, ChannelConfig *chans, SampleType *type);

        virtual size_t read(char *buffer, size_t bytes);
        virtual void readAll(std::vector<char> &output) {}
        virtual size_t getSampleOffset() { return 0; }

        VMO_Generator();
        virtual ~VMO_Generator() {}
    };

}

#endif /* MWSOUND_VMO_GENERATOR_HPP_ */
