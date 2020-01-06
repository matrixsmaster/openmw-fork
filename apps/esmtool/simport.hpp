#pragma once

#include <iostream>
#include <sstream>
#include <boost/format.hpp>
#include "record.hpp"
#include "labels.hpp"

namespace EsmTool {

template<>
void Record<ESM::Activator>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Potion>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Armor>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Apparatus>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::BodyPart>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Book>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
    mData.mText = *cit++;
}

template<>
void Record<ESM::BirthSign>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
    mData.mDescription = *cit++;
}

template<>
void Record<ESM::Cell>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Class>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
    mData.mDescription = *cit++;
}

template<>
void Record<ESM::Clothing>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Container>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Creature>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Dialogue>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Door>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Enchantment>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Faction>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
    for (int i = 0; i != 10; i++)
        mData.mRanks[i] = *cit++;
}

template<>
void Record<ESM::Global>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::GameSetting>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::DialInfo>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mResponse = *cit++;
    mData.mResultScript = *cit++;
}

template<>
void Record<ESM::Ingredient>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Land>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::CreatureLevList>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::ItemLevList>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Light>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Lockpick>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Probe>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Repair>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::LandTexture>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::MagicEffect>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mDescription = *cit++;
}

template<>
void Record<ESM::Miscellaneous>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::NPC>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Pathgrid>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Race>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
    mData.mDescription = *cit++;
}

template<>
void Record<ESM::Region>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::Script>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mScriptText = *cit++;
}

template<>
void Record<ESM::Skill>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mDescription = *cit++;
}

template<>
void Record<ESM::SoundGenerator>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Sound>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Spell>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

template<>
void Record<ESM::StartScript>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Static>::fromtext(std::vector<std::string>::iterator &cit)
{
}

template<>
void Record<ESM::Weapon>::fromtext(std::vector<std::string>::iterator &cit)
{
    mData.mName = *cit++;
}

} //EsmTool
