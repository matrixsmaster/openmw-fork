#ifndef GAME_MWBASE_INPUTMANAGER_H
#define GAME_MWBASE_INPUTMANAGER_H

#include <string>
#include <set>
#include <vector>

#include <stdint.h>

#include <extern/xswrapper/eventsink.hpp>

namespace Loading
{
    class Listener;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace MWBase
{
    /// \brief Interface for input manager(implemented in MWInput)
    class InputManager
    {
            ///< not implemented
            InputManager(const InputManager&);

            ///< not implemented
            InputManager& operator= (const InputManager&);

        public:

            InputManager() {}

            /// Clear all savegame-specific data
            virtual void clear() = 0;

            virtual ~InputManager() {}

            virtual bool isWindowVisible() = 0;

            virtual void update(float dt, bool disableControls, bool disableEvents=false) = 0;

            virtual void changeInputMode(bool guiMode) = 0;

            virtual void processChangedSettings(const std::set< std::pair<std::string, std::string> >& changed) = 0;

            virtual void setDragDrop(bool dragDrop) = 0;

            virtual void toggleControlSwitch(const std::string& sw, bool value) = 0;
            virtual bool getControlSwitch(const std::string& sw) = 0;

            virtual std::string getActionDescription(int action) = 0;
            virtual std::string getActionKeyBindingName(int action) = 0;
            virtual std::vector<int> getActionKeySorting() = 0;
            virtual int getNumActions() = 0;
            virtual void enableDetectingBindingMode(int action) = 0;
            virtual void resetToDefaultKeyBindings() = 0;

            virtual int countSavedGameRecords() const = 0;
            virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) = 0;
            virtual void readRecord(ESM::ESMReader& reader, uint32_t type) = 0;

            virtual void setEventSinks(wrapperEventSinkType* to) = 0;
    };
}

#endif
