#include <iostream>
#include <sstream>
#include <boost/format.hpp>

#include "record.hpp"
#include "labels.hpp"
#include "printers.hpp"
#include "dumpers.hpp"

using namespace EsmTool;

RecordBase *
RecordBase::create(ESM::NAME type)
{
    RecordBase *record = 0;

    switch (type.intval) {
    case ESM::REC_ACTI:
    {
        record = new EsmTool::Record<ESM::Activator>;
        break;
    }
    case ESM::REC_ALCH:
    {
        record = new EsmTool::Record<ESM::Potion>;
        break;
    }
    case ESM::REC_APPA:
    {
        record = new EsmTool::Record<ESM::Apparatus>;
        break;
    }
    case ESM::REC_ARMO:
    {
        record = new EsmTool::Record<ESM::Armor>;
        break;
    }
    case ESM::REC_BODY:
    {
        record = new EsmTool::Record<ESM::BodyPart>;
        break;
    }
    case ESM::REC_BOOK:
    {
        record = new EsmTool::Record<ESM::Book>;
        break;
    }
    case ESM::REC_BSGN:
    {
        record = new EsmTool::Record<ESM::BirthSign>;
        break;
    }
    case ESM::REC_CELL:
    {
        record = new EsmTool::Record<ESM::Cell>;
        break;
    }
    case ESM::REC_CLAS:
    {
        record = new EsmTool::Record<ESM::Class>;
        break;
    }
    case ESM::REC_CLOT:
    {
        record = new EsmTool::Record<ESM::Clothing>;
        break;
    }
    case ESM::REC_CONT:
    {
        record = new EsmTool::Record<ESM::Container>;
        break;
    }
    case ESM::REC_CREA:
    {
        record = new EsmTool::Record<ESM::Creature>;
        break;
    }
    case ESM::REC_DIAL:
    {
        record = new EsmTool::Record<ESM::Dialogue>;
        break;
    }
    case ESM::REC_DOOR:
    {
        record = new EsmTool::Record<ESM::Door>;
        break;
    }
    case ESM::REC_ENCH:
    {
        record = new EsmTool::Record<ESM::Enchantment>;
        break;
    }
    case ESM::REC_FACT:
    {
        record = new EsmTool::Record<ESM::Faction>;
        break;
    }
    case ESM::REC_GLOB:
    {
        record = new EsmTool::Record<ESM::Global>;
        break;
    }
    case ESM::REC_GMST:
    {
        record = new EsmTool::Record<ESM::GameSetting>;
        break;
    }
    case ESM::REC_INFO:
    {
        record = new EsmTool::Record<ESM::DialInfo>;
        break;
    }
    case ESM::REC_INGR:
    {
        record = new EsmTool::Record<ESM::Ingredient>;
        break;
    }
    case ESM::REC_LAND:
    {
        record = new EsmTool::Record<ESM::Land>;
        break;
    }
    case ESM::REC_LEVI:
    {
        record = new EsmTool::Record<ESM::ItemLevList>;
        break;
    }
    case ESM::REC_LEVC:
    {
        record = new EsmTool::Record<ESM::CreatureLevList>;
        break;
    }
    case ESM::REC_LIGH:
    {
        record = new EsmTool::Record<ESM::Light>;
        break;
    }
    case ESM::REC_LOCK:
    {
        record = new EsmTool::Record<ESM::Lockpick>;
        break;
    }
    case ESM::REC_LTEX:
    {
        record = new EsmTool::Record<ESM::LandTexture>;
        break;
    }
    case ESM::REC_MISC:
    {
        record = new EsmTool::Record<ESM::Miscellaneous>;
        break;
    }
    case ESM::REC_MGEF:
    {
        record = new EsmTool::Record<ESM::MagicEffect>;
        break;
    }
    case ESM::REC_NPC_:
    {
        record = new EsmTool::Record<ESM::NPC>;
        break;
    }
    case ESM::REC_PGRD:
    {
        record = new EsmTool::Record<ESM::Pathgrid>;
        break;
    }
    case ESM::REC_PROB:
    {
        record = new EsmTool::Record<ESM::Probe>;
        break;
    }
    case ESM::REC_RACE:
    {
        record = new EsmTool::Record<ESM::Race>;
        break;
    }
    case ESM::REC_REGN:
    {
        record = new EsmTool::Record<ESM::Region>;
        break;
    }
    case ESM::REC_REPA:
    {
        record = new EsmTool::Record<ESM::Repair>;
        break;
    }
    case ESM::REC_SCPT:
    {
        record = new EsmTool::Record<ESM::Script>;
        break;
    }
    case ESM::REC_SKIL:
    {
        record = new EsmTool::Record<ESM::Skill>;
        break;
    }
    case ESM::REC_SNDG:
    {
        record = new EsmTool::Record<ESM::SoundGenerator>;
        break;
    }
    case ESM::REC_SOUN:
    {
        record = new EsmTool::Record<ESM::Sound>;
        break;
    }
    case ESM::REC_SPEL:
    {
        record = new EsmTool::Record<ESM::Spell>;
        break;
    }
    case ESM::REC_STAT:
    {
        record = new EsmTool::Record<ESM::Static>;
        break;
    }
    case ESM::REC_WEAP:
    {
        record = new EsmTool::Record<ESM::Weapon>;
        break;
    }
    case ESM::REC_SSCR:
    {
        record = new EsmTool::Record<ESM::StartScript>;
        break;
    }
    default:
        record = 0;
    }
    if (record) {
        record->mType = type;
    }
    return record;
}

template<>
std::string Record<ESM::Cell>::getId() const
{
    return mData.mName;
}

template<>
std::string Record<ESM::Land>::getId() const
{
    return ""; // No ID for Land record
}

template<>
std::string Record<ESM::MagicEffect>::getId() const
{
    return ""; // No ID for MagicEffect record
}

template<>
std::string Record<ESM::Pathgrid>::getId() const
{
    return ""; // No ID for Pathgrid record
}

template<>
std::string Record<ESM::Skill>::getId() const
{
    return ""; // No ID for Skill record
}
