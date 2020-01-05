#pragma once

#include <iostream>
#include <sstream>
#include <boost/format.hpp>
#include "record.hpp"
#include "labels.hpp"

namespace EsmTool {

template<>
void Record<ESM::Activator>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Potion>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Armor>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Apparatus>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::BodyPart>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::Book>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
    coll.push_back(mData.mText);
}

template<>
void Record<ESM::BirthSign>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
    coll.push_back(mData.mDescription);
}

template<>
void Record<ESM::Cell>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::Class>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
    coll.push_back(mData.mDescription);
}

template<>
void Record<ESM::Clothing>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Container>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Creature>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Dialogue>::textdump(std::vector<std::string> &coll)
{
#if 0
    ESM::Dialogue::InfoContainer::iterator iit;
    for (iit = mData.mInfo.begin(); iit != mData.mInfo.end(); ++iit)
        std::cout << "INFO!" << iit->mId << std::endl;
#endif
}

template<>
void Record<ESM::Door>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Enchantment>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::Faction>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
    for (int i = 0; i != 10; i++)
        coll.push_back(mData.mRanks[i]);
}

template<>
void Record<ESM::Global>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::GameSetting>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::DialInfo>::textdump(std::vector<std::string> &coll)
{
//    coll.push_back(mData.mId);
//    coll.push_back(mData.mPrev);
//    coll.push_back(mData.mNext);
    coll.push_back(mData.mResponse);
    coll.push_back(mData.mResultScript);
}

template<>
void Record<ESM::Ingredient>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Land>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::CreatureLevList>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::ItemLevList>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::Light>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Lockpick>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Probe>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Repair>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::LandTexture>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::MagicEffect>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mDescription);
}

template<>
void Record<ESM::Miscellaneous>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::NPC>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Pathgrid>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::Race>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
    coll.push_back(mData.mDescription);
}

template<>
void Record<ESM::Region>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::Script>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mScriptText);
}

template<>
void Record<ESM::Skill>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mDescription);
}

template<>
void Record<ESM::SoundGenerator>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::Sound>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::Spell>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

template<>
void Record<ESM::StartScript>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::Static>::textdump(std::vector<std::string> &coll)
{
}

template<>
void Record<ESM::Weapon>::textdump(std::vector<std::string> &coll)
{
    coll.push_back(mData.mName);
}

} //EsmTool
