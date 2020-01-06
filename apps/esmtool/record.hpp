#ifndef OPENMW_ESMTOOL_RECORD_H
#define OPENMW_ESMTOOL_RECORD_H

#include <string>

#include <components/esm/records.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace EsmTool
{
    template <class T> class Record;

    class RecordBase
    {
    protected:
        std::string mId;
        uint32_t mFlags;
        ESM::NAME mType;
        bool mPrintPlain;

    public:
        RecordBase ()
          : mFlags(0)
          , mPrintPlain(false)
        {
        }

        virtual ~RecordBase() {}

        virtual std::string getId() const = 0;
        virtual void setId(std::string id) = 0;

        virtual std::string getName() const = 0;
        virtual void setName(std::string n) = 0;

        uint32_t getFlags() const {
            return mFlags;
        }

        void setFlags(uint32_t flags) {
            mFlags = flags;
        }

        ESM::NAME getType() const {
            return mType;
        }

        void setPrintPlain(bool plain) {
            mPrintPlain = plain;
        }

        virtual void load(ESM::ESMReader &esm) = 0;
        virtual void save(ESM::ESMWriter &esm) = 0;
        virtual void print() = 0;

        virtual void textdump(std::vector<std::string> &coll) = 0;
        virtual void fromtext(std::vector<std::string>::iterator &cit) = 0;

        static RecordBase *create(ESM::NAME type);

        // just make it a bit shorter
        template <class T>
        Record<T> *cast() {
            return static_cast<Record<T> *>(this);
        }
    };

    template <class T>
    class Record : public RecordBase
    {
        T mData;
        bool mIsDeleted;

    public:
        Record()
            : mIsDeleted(false)
        {}

        std::string getId() const {
            return mData.mId;
        }
        void setId(std::string id) {
            mData.mId = id;
        }

        std::string getName() const {
            return mData.mName;
        }
        void setName(std::string n) {
            mData.mName = n;
        }

        T &get() {
            return mData;
        }

        void save(ESM::ESMWriter &esm) {
            mData.save(esm, mIsDeleted);
        }

        void load(ESM::ESMReader &esm) {
            mData.load(esm, mIsDeleted);
        }

        void print();
        void textdump(std::vector<std::string> &coll);
        void fromtext(std::vector<std::string>::iterator &cit);
    };

    template<> std::string Record<ESM::Cell>::getId() const;
    template<> std::string Record<ESM::Land>::getId() const;
    template<> std::string Record<ESM::MagicEffect>::getId() const;
    template<> std::string Record<ESM::Pathgrid>::getId() const;
    template<> std::string Record<ESM::Skill>::getId() const;

    template<> void Record<ESM::Cell>::setId(std::string id);
    template<> void Record<ESM::Land>::setId(std::string id);
    template<> void Record<ESM::MagicEffect>::setId(std::string id);
    template<> void Record<ESM::Pathgrid>::setId(std::string id);
    template<> void Record<ESM::Skill>::setId(std::string id);

    template<> std::string Record<ESM::BodyPart>::getName() const;
    template<> std::string Record<ESM::CreatureLevList>::getName() const;
    template<> std::string Record<ESM::Dialogue>::getName() const;
    template<> std::string Record<ESM::DialInfo>::getName() const;
    template<> std::string Record<ESM::Enchantment>::getName() const;
    template<> std::string Record<ESM::Global>::getName() const;
    template<> std::string Record<ESM::GameSetting>::getName() const;
    template<> std::string Record<ESM::ItemLevList>::getName() const;
    template<> std::string Record<ESM::Land>::getName() const;
    template<> std::string Record<ESM::LandTexture>::getName() const;
    template<> std::string Record<ESM::MagicEffect>::getName() const;
    template<> std::string Record<ESM::Pathgrid>::getName() const;
    template<> std::string Record<ESM::Script>::getName() const;
    template<> std::string Record<ESM::StartScript>::getName() const;
    template<> std::string Record<ESM::Skill>::getName() const;
    template<> std::string Record<ESM::Sound>::getName() const;
    template<> std::string Record<ESM::SoundGenerator>::getName() const;
    template<> std::string Record<ESM::Static>::getName() const;

    template<> void Record<ESM::BodyPart>::setName(std::string n);
    template<> void Record<ESM::CreatureLevList>::setName(std::string n);
    template<> void Record<ESM::Dialogue>::setName(std::string n);
    template<> void Record<ESM::DialInfo>::setName(std::string n);
    template<> void Record<ESM::Enchantment>::setName(std::string n);
    template<> void Record<ESM::Global>::setName(std::string n);
    template<> void Record<ESM::GameSetting>::setName(std::string n);
    template<> void Record<ESM::ItemLevList>::setName(std::string n);
    template<> void Record<ESM::Land>::setName(std::string n);
    template<> void Record<ESM::LandTexture>::setName(std::string n);
    template<> void Record<ESM::MagicEffect>::setName(std::string n);
    template<> void Record<ESM::Pathgrid>::setName(std::string n);
    template<> void Record<ESM::Script>::setName(std::string n);
    template<> void Record<ESM::StartScript>::setName(std::string n);
    template<> void Record<ESM::Skill>::setName(std::string n);
    template<> void Record<ESM::Sound>::setName(std::string n);
    template<> void Record<ESM::SoundGenerator>::setName(std::string n);
    template<> void Record<ESM::Static>::setName(std::string n);

    template<> void Record<ESM::Activator>::print();
    template<> void Record<ESM::Potion>::print();
    template<> void Record<ESM::Armor>::print();
    template<> void Record<ESM::Apparatus>::print();
    template<> void Record<ESM::BodyPart>::print();
    template<> void Record<ESM::Book>::print();
    template<> void Record<ESM::BirthSign>::print();
    template<> void Record<ESM::Cell>::print();
    template<> void Record<ESM::Class>::print();
    template<> void Record<ESM::Clothing>::print();
    template<> void Record<ESM::Container>::print();
    template<> void Record<ESM::Creature>::print();
    template<> void Record<ESM::Dialogue>::print();
    template<> void Record<ESM::Door>::print();
    template<> void Record<ESM::Enchantment>::print();
    template<> void Record<ESM::Faction>::print();
    template<> void Record<ESM::Global>::print();
    template<> void Record<ESM::GameSetting>::print();
    template<> void Record<ESM::DialInfo>::print();
    template<> void Record<ESM::Ingredient>::print();
    template<> void Record<ESM::Land>::print();
    template<> void Record<ESM::CreatureLevList>::print();
    template<> void Record<ESM::ItemLevList>::print();
    template<> void Record<ESM::Light>::print();
    template<> void Record<ESM::Lockpick>::print();
    template<> void Record<ESM::Probe>::print();
    template<> void Record<ESM::Repair>::print();
    template<> void Record<ESM::LandTexture>::print();
    template<> void Record<ESM::MagicEffect>::print();
    template<> void Record<ESM::Miscellaneous>::print();
    template<> void Record<ESM::NPC>::print();
    template<> void Record<ESM::Pathgrid>::print();
    template<> void Record<ESM::Race>::print();
    template<> void Record<ESM::Region>::print();
    template<> void Record<ESM::Script>::print();
    template<> void Record<ESM::Skill>::print();
    template<> void Record<ESM::SoundGenerator>::print();
    template<> void Record<ESM::Sound>::print();
    template<> void Record<ESM::Spell>::print();
    template<> void Record<ESM::StartScript>::print();
    template<> void Record<ESM::Static>::print();
    template<> void Record<ESM::Weapon>::print();

    template<> void Record<ESM::Activator>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Potion>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Armor>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Apparatus>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::BodyPart>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Book>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::BirthSign>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Cell>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Class>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Clothing>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Container>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Creature>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Dialogue>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Door>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Enchantment>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Faction>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Global>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::GameSetting>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::DialInfo>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Ingredient>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Land>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::CreatureLevList>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::ItemLevList>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Light>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Lockpick>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Probe>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Repair>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::LandTexture>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::MagicEffect>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Miscellaneous>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::NPC>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Pathgrid>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Race>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Region>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Script>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Skill>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::SoundGenerator>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Sound>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Spell>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::StartScript>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Static>::textdump(std::vector<std::string> &coll);
    template<> void Record<ESM::Weapon>::textdump(std::vector<std::string> &coll);

    template<> void Record<ESM::Activator>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Potion>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Armor>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Apparatus>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::BodyPart>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Book>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::BirthSign>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Cell>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Class>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Clothing>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Container>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Creature>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Dialogue>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Door>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Enchantment>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Faction>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Global>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::GameSetting>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::DialInfo>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Ingredient>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Land>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::CreatureLevList>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::ItemLevList>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Light>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Lockpick>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Probe>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Repair>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::LandTexture>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::MagicEffect>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Miscellaneous>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::NPC>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Pathgrid>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Race>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Region>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Script>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Skill>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::SoundGenerator>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Sound>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Spell>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::StartScript>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Static>::fromtext(std::vector<std::string>::iterator &cit);
    template<> void Record<ESM::Weapon>::fromtext(std::vector<std::string>::iterator &cit);
}

#endif
