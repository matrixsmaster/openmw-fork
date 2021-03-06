#include <iostream>
#include <iomanip>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <fstream>
#include <cmath>
#include <algorithm>

#include <boost/program_options.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/records.hpp>

#include "record.hpp"

#define ESMTOOL_VERSION 1.3

// Create a local alias for brevity
namespace bpo = boost::program_options;

struct ESMData
{
    std::string author;
    std::string description;
    unsigned int version;
    std::vector<ESM::Header::MasterData> masters;

    std::deque<EsmTool::RecordBase *> mRecords;
    // Value: (Reference, Deleted flag)
    std::map<ESM::Cell *, std::deque<std::pair<ESM::CellRef, bool> > > mCellRefs;
    std::map<int, int> mRecordStats;

    static const std::set<int> sLabeledRec;
};

static const int sLabeledRecIds[] = {
    ESM::REC_GLOB, ESM::REC_CLAS, ESM::REC_FACT, ESM::REC_RACE, ESM::REC_SOUN,
    ESM::REC_REGN, ESM::REC_BSGN, ESM::REC_LTEX, ESM::REC_STAT, ESM::REC_DOOR,
    ESM::REC_MISC, ESM::REC_WEAP, ESM::REC_CONT, ESM::REC_SPEL, ESM::REC_CREA,
    ESM::REC_BODY, ESM::REC_LIGH, ESM::REC_ENCH, ESM::REC_NPC_, ESM::REC_ARMO,
    ESM::REC_CLOT, ESM::REC_REPA, ESM::REC_ACTI, ESM::REC_APPA, ESM::REC_LOCK,
    ESM::REC_PROB, ESM::REC_INGR, ESM::REC_BOOK, ESM::REC_ALCH, ESM::REC_LEVI,
    ESM::REC_LEVC, ESM::REC_SNDG, ESM::REC_CELL, ESM::REC_DIAL
};

const std::set<int> ESMData::sLabeledRec =
    std::set<int>(sLabeledRecIds, sLabeledRecIds + 34);

// Based on the legacy struct
struct Arguments
{
    bool raw_given;
    bool quiet_given;
    bool loadcells_given;
    bool plain_given;

    std::string mode;
    std::string encoding;
    std::string filename;
    std::string outname;

    std::vector<std::string> types;
    std::string name;
    std::string readback;
    std::string master;

    ESMData data;
    ESM::ESMReader reader;
    ESM::ESMWriter writer;

    std::vector<std::string> storage;
};

bool parseOptions (int argc, char** argv, Arguments &info)
{
    bpo::options_description desc(  "Inspect and extract from Morrowind ES files (ESM, ESP, ESS)\n"
                                    "Syntax: esmtool [options] mode infile [outfile]\n"
                                    "Allowed modes:\n"
                                    "\tdump\t Dumps all readable data from the input file.\n"
                                    "\tclone\t Clones the input file to the output file.\n"
                                    "\tcomp\t Compares the given files.\n"
                                    "\tstrings\t Dumps (or reads back) all strings and textual information.\n"
                                    "\trevert\t Reverts all FNAMs within a plugin back to original values.\n\n"
                                    "Allowed options:"
                                );

    desc.add_options()
        ("help,h", "print help message.")
        ("version,v", "print version information and quit.")
        ("raw,r", "Show an unformatted list of all records and subrecords.")
        // The intention is that this option would interact better
        // with other modes including clone, dump, and raw.
        ("type,t", bpo::value< std::vector<std::string> >(),
         "Show only records of this type (four character record code).  May "
         "be specified multiple times.  Only affects dump mode.")
        ("name,n", bpo::value<std::string>(),
         "Show only the record with this name.  Only affects dump mode.")
        ("plain,p", "Print contents of dialogs, books and scripts. "
         "(skipped by default)"
         "Only affects dump mode.")
        ("quiet,q", "Supress all record information. Useful for speed tests.")
        ("loadcells,C", "Browse through contents of all cells.")
        ("readback,B",  bpo::value<std::string>(),
         "Read-back the strings dump from a file to recreate a new esp file.")
     	("master,M",  bpo::value<std::string>(),
		 "Master file (not necessarily ESM) to revert data from.")

        ( "encoding,e", bpo::value<std::string>(&(info.encoding))->
          default_value("win1252"),
          "Character encoding used in ESMTool:\n"
          "\n\twin1250 - Central and Eastern European such as Polish, Czech, Slovak, Hungarian, Slovene, Bosnian, Croatian, Serbian (Latin script), Romanian and Albanian languages\n"
          "\n\twin1251 - Cyrillic alphabet such as Russian, Bulgarian, Serbian Cyrillic and other languages\n"
          "\n\twin1252 - Western European (Latin) alphabet, used by default")
        ;

    std::string finalText = "\nIf no option is given, the default action is to parse all records in the archive\nand display diagnostic information.";

    // input-file is hidden and used as a positional argument
    bpo::options_description hidden("Hidden Options");

    hidden.add_options()
        ( "mode,m", bpo::value<std::string>(), "esmtool mode")
        ( "input-file,i", bpo::value< std::vector<std::string> >(), "input file")
        ;

    bpo::positional_options_description p;
    p.add("mode", 1).add("input-file", 2);

    // there might be a better way to do this
    bpo::options_description all;
    all.add(desc).add(hidden);
    bpo::variables_map variables;

    try
    {
        bpo::parsed_options valid_opts = bpo::command_line_parser(argc, argv)
            .options(all).positional(p).run();

        bpo::store(valid_opts, variables);
    }
    catch(boost::program_options::unknown_option & x)
    {
        std::cerr << "ERROR: " << x.what() << std::endl;
        return false;
    }
    catch(boost::program_options::invalid_command_line_syntax & x)
    {
        std::cerr << "ERROR: " << x.what() << std::endl;
        return false;
    }

    bpo::notify(variables);

    if (variables.count ("help"))
    {
        std::cout << desc << finalText << std::endl;
        return false;
    }
    if (variables.count ("version"))
    {
        std::cout << "ESMTool version " << ESMTOOL_VERSION << std::endl;
        return false;
    }
    if (!variables.count("mode"))
    {
        std::cout << "No mode specified!" << std::endl << std::endl
                  << desc << finalText << std::endl;
        return false;
    }

    if (variables.count("type") > 0)
        info.types = variables["type"].as< std::vector<std::string> >();
    if (variables.count("name") > 0)
        info.name = variables["name"].as<std::string>();
    if (variables.count("readback") > 0)
        info.readback = variables["readback"].as<std::string>();
    if (variables.count("master") > 0)
        info.master = variables["master"].as<std::string>();

    info.mode = variables["mode"].as<std::string>(); // checked later, in main mode switch

    if ( !variables.count("input-file") )
    {
        std::cout << "\nERROR: missing ES file\n\n";
        std::cout << desc << finalText << std::endl;
        return false;
    }

    info.filename = variables["input-file"].as< std::vector<std::string> >()[0];
    if (variables["input-file"].as< std::vector<std::string> >().size() > 1)
        info.outname = variables["input-file"].as< std::vector<std::string> >()[1];

    info.raw_given = variables.count ("raw") != 0;
    info.quiet_given = variables.count ("quiet") != 0;
    info.loadcells_given = variables.count ("loadcells") != 0;
    info.plain_given = variables.count("plain") != 0;

    // Font encoding settings
    info.encoding = variables["encoding"].as<std::string>();
    if(info.encoding != "win1250" && info.encoding != "win1251" && info.encoding != "win1252")
    {
        std::cout << info.encoding << " is not a valid encoding option." << std::endl;
        info.encoding = "win1252";
    }
    std::cout << ToUTF8::encodingUsingMessage(info.encoding) << std::endl;

    return true;
}

void loadCell(ESM::Cell &cell, ESM::ESMReader &esm, Arguments& info)
{
    bool quiet = (info.quiet_given || info.mode == "clone");
    bool save = (info.mode == "clone");

    // Skip back to the beginning of the reference list
    // FIXME: Changes to the references backend required to support multiple plugins have
    //  almost certainly broken this following line. I'll leave it as is for now, so that
    //  the compiler does not complain.
    cell.restore(esm, 0);

    // Loop through all the references
    ESM::CellRef ref;
    if(!quiet) std::cout << "  References:\n";

    bool deleted = false;
    while(cell.getNextRef(esm, ref, deleted))
    {
        if (save) {
            info.data.mCellRefs[&cell].push_back(std::make_pair(ref, deleted));
        }

        if(quiet) continue;

        std::cout << "    Refnum: " << ref.mRefNum.mIndex << std::endl;
        std::cout << "    ID: '" << ref.mRefID << "'\n";
        std::cout << "    Owner: '" << ref.mOwner << "'\n";
        std::cout << "    Global: '" << ref.mGlobalVariable << "'" << std::endl;
        std::cout << "    Faction: '" << ref.mFaction << "'" << std::endl;
        std::cout << "    Faction rank: '" << ref.mFactionRank << "'" << std::endl;
        std::cout << "    Enchantment charge: '" << ref.mEnchantmentCharge << "'\n";
        std::cout << "    Uses/health: '" << ref.mChargeInt << "'\n";
        std::cout << "    Gold value: '" << ref.mGoldValue << "'\n";
        std::cout << "    Blocked: '" << static_cast<int>(ref.mReferenceBlocked) << "'" << std::endl;
        std::cout << "    Deleted: " << deleted << std::endl;
        if (!ref.mKey.empty())
            std::cout << "    Key: '" << ref.mKey << "'" << std::endl;
    }
}

void printRaw(ESM::ESMReader &esm)
{
    while(esm.hasMoreRecs())
    {
        ESM::NAME n = esm.getRecName();
        std::cout << "Record: " << n.toString() << std::endl;
        esm.getRecHeader();
        while(esm.hasMoreSubs())
        {
            size_t offs = esm.getFileOffset();
            esm.getSubName();
            esm.skipHSub();
            n = esm.retSubName();
            std::ios::fmtflags f(std::cout.flags());
            std::cout << "    " << n.toString() << " - " << esm.getSubSize()
                 << " bytes @ 0x" << std::hex << offs << "\n";
            std::cout.flags(f);
        }
    }
}

int load(Arguments& info)
{
    ESM::ESMReader& esm = info.reader;
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding(info.encoding));
    esm.setEncoder(&encoder);

    std::string filename = info.filename;
    std::cout << "Loading file: " << filename << std::endl;

    std::list<int> skipped;

    try {

        if(info.raw_given && info.mode == "dump")
        {
            std::cout << "RAW file listing:\n";

            esm.openRaw(filename);

            printRaw(esm);

            return 0;
        }

        bool quiet = (info.quiet_given || info.mode == "clone");
        bool loadCells = (info.loadcells_given || info.mode == "clone");
        bool save = (info.mode == "clone");

        esm.open(filename);

        info.data.author = esm.getAuthor();
        info.data.description = esm.getDesc();
        info.data.masters = esm.getGameFiles();

        if (!quiet)
        {
            std::cout << "Author: " << esm.getAuthor() << std::endl
                 << "Description: " << esm.getDesc() << std::endl
                 << "File format version: " << esm.getFVer() << std::endl;
            std::vector<ESM::Header::MasterData> m = esm.getGameFiles();
            if (!m.empty())
            {
                std::cout << "Masters:" << std::endl;
                for(unsigned int i=0;i<m.size();i++)
                    std::cout << "  " << m[i].name << ", " << m[i].size << " bytes" << std::endl;
            }
        }

        // Loop through all records
        while(esm.hasMoreRecs())
        {
            ESM::NAME n = esm.getRecName();
            uint32_t flags;
            esm.getRecHeader(flags);

            EsmTool::RecordBase *record = EsmTool::RecordBase::create(n);
            if (record == 0)
            {
                if (std::find(skipped.begin(), skipped.end(), n.intval) == skipped.end())
                {
                    std::cout << "Skipping " << n.toString() << " records." << std::endl;
                    skipped.push_back(n.intval);
                }

                esm.skipRecord();
                if (quiet) break;
                std::cout << "  Skipping\n";

                continue;
            }

            record->setFlags(static_cast<int>(flags));
            record->setPrintPlain(info.plain_given);
            record->load(esm);

            // Is the user interested in this record type?
            bool interested = true;
            if (!info.types.empty())
            {
                std::vector<std::string>::iterator match;
                match = std::find(info.types.begin(), info.types.end(), n.toString());
                if (match == info.types.end()) interested = false;
            }

            if (!info.name.empty() && !Misc::StringUtils::ciEqual(info.name, record->getId()))
                interested = false;

            if(!quiet && interested)
            {
                std::cout << "\nRecord: " << n.toString() << " '" << record->getId() << "'\n";
                record->print();
            }

            if (record->getType().intval == ESM::REC_CELL && loadCells && interested)
            {
                loadCell(record->cast<ESM::Cell>()->get(), esm, info);
            }

            if (save)
            {
                info.data.mRecords.push_back(record);
            }
            else
            {
                delete record;
            }
            ++info.data.mRecordStats[n.intval];
        }

    } catch(std::exception &e) {
        std::cout << "\nERROR:\n\n  " << e.what() << std::endl;

        typedef std::deque<EsmTool::RecordBase *> RecStore;
        RecStore &store = info.data.mRecords;
        for (RecStore::iterator it = store.begin(); it != store.end(); ++it)
        {
            delete *it;
        }
        store.clear();
        return 1;
    }

    return 0;
}

int clone(Arguments& info)
{
    if (info.outname.empty())
    {
        std::cout << "You need to specify an output name" << std::endl;
        return 1;
    }

    if (load(info) != 0)
    {
        std::cout << "Failed to load, aborting." << std::endl;
        return 1;
    }

    size_t recordCount = info.data.mRecords.size();

    int digitCount = 1; // For a nicer output
    if (recordCount > 0)
        digitCount = (int)std::log10(recordCount) + 1;

    std::cout << "Loaded " << recordCount << " records:" << std::endl << std::endl;

    int i = 0;
    typedef std::map<int, int> Stats;
    Stats &stats = info.data.mRecordStats;
    for (Stats::iterator it = stats.begin(); it != stats.end(); ++it)
    {
        ESM::NAME name;
        name.intval = it->first;
        int amount = it->second;
        std::cout << std::setw(digitCount) << amount << " " << name.toString() << "  ";

        if (++i % 3 == 0)
            std::cout << std::endl;
    }

    if (i % 3 != 0)
        std::cout << std::endl;

    std::cout << std::endl << "Saving records to: " << info.outname << "..." << std::endl;

    ESM::ESMWriter& esm = info.writer;
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding(info.encoding));
    esm.setEncoder(&encoder);
    esm.setAuthor(info.data.author);
    esm.setDescription(info.data.description);
    esm.setVersion(info.data.version);
    esm.setRecordCount (recordCount);

    for (std::vector<ESM::Header::MasterData>::iterator it = info.data.masters.begin(); it != info.data.masters.end(); ++it)
        esm.addMaster(it->name, it->size);

    std::fstream save(info.outname.c_str(), std::fstream::out | std::fstream::binary);
    esm.save(save);

    int saved = 0;
    typedef std::deque<EsmTool::RecordBase *> Records;
    Records &records = info.data.mRecords;
    for (Records::iterator it = records.begin(); it != records.end() && i > 0; ++it)
    {
        EsmTool::RecordBase *record = *it;
        const ESM::NAME& typeName = record->getType();

        esm.startRecord(typeName.toString(), record->getFlags());

        record->save(esm);
        if (typeName.intval == ESM::REC_CELL) {
            ESM::Cell *ptr = &record->cast<ESM::Cell>()->get();
            if (!info.data.mCellRefs[ptr].empty()) {
                typedef std::deque<std::pair<ESM::CellRef, bool> > RefList;
                RefList &refs = info.data.mCellRefs[ptr];
                for (RefList::iterator refIt = refs.begin(); refIt != refs.end(); ++refIt)
                {
                    refIt->first.save(esm, refIt->second);
                }
            }
        }

        esm.endRecord(typeName.toString());

        saved++;
        int perc = (int)((saved / (float)recordCount)*100);
        if (perc % 10 == 0)
        {
            std::cout << "\r" << perc << "%";
        }
    }

    std::cout << "\rDone!" << std::endl;

    esm.close();
    save.close();

    return 0;
}

int comp(Arguments& info)
{
    if (info.filename.empty() || info.outname.empty())
    {
        std::cout << "You need to specify two input files" << std::endl;
        return 1;
    }

    Arguments fileOne;
    Arguments fileTwo;

    fileOne.raw_given = 0;
    fileTwo.raw_given = 0;

    fileOne.mode = "clone";
    fileTwo.mode = "clone";

    fileOne.encoding = info.encoding;
    fileTwo.encoding = info.encoding;

    fileOne.filename = info.filename;
    fileTwo.filename = info.outname;

    if (load(fileOne) != 0)
    {
        std::cout << "Failed to load " << info.filename << ", aborting comparison." << std::endl;
        return 1;
    }

    if (load(fileTwo) != 0)
    {
        std::cout << "Failed to load " << info.outname << ", aborting comparison." << std::endl;
        return 1;
    }

    if (fileOne.data.mRecords.size() != fileTwo.data.mRecords.size())
    {
        std::cout << "Not equal, different amount of records." << std::endl;
        return 1;
    }

    return 0;
}

int readback_line(Arguments& info, int num, std::string content)
{
	if (num < 0) return 0;
	if (num >= int(info.storage.size())) return 1;

	if (content.at(content.size()-1) == '\n') content.resize(content.size()-1);

	printf("DEBUG: Num %d String '%s'\n",num,content.c_str());
	info.storage[num] = content;

	return 0;
}

int readback_file(Arguments& info)
{
    if (info.readback.empty()) return 0;

    std::ifstream mfl(info.readback);
    if (!mfl.is_open()) {
        std::cerr << "ERROR: Unable to open file " << info.readback << std::endl;
        return 1;
    }

    int num = -1;
    int fsm = 0;
    int ch;
    std::string accum;

    while (1) {
    	ch = mfl.get();
    	if (mfl.eof()) break;

    	switch (fsm) {
    	case 0:
    		if (ch == '$' && mfl.peek() == '$') {
    			if (readback_line(info,num,accum)) {
    				mfl.close();
    				return 1;
    			}
    			accum.clear();
    			mfl.get();
    			fsm = 1;
    		} else
    			accum += ch;
    		break;
    	case 1:
    		if (ch == ' ') {
    			int tmp = atoi(accum.c_str());
    			if (tmp != ++num) {
    				printf("ERROR: Line numbers are out of sync: epxected %d, got %d\n",num,tmp);
    				mfl.close();
    				return 1;
    			}
    			num = tmp;
    			accum.clear();
    			fsm = 0;
    		} else
    			accum += ch;
    		break;
    	default:
    		abort();
    	}
    }
    mfl.close();

    if (fsm == 0 && !accum.empty())
    	return readback_line(info,num,accum);
    else
    	return 0;
}

int readback(Arguments& info)
{
    if (info.outname.empty()) {
        std::cout << "You need to specify an output name" << std::endl;
        return 1;
    }

    if (readback_file(info)) return 1;

    std::cout << std::endl << "Saving records to: " << info.outname << "..." << std::endl;

    std::vector<std::string>::iterator sit = info.storage.begin();
    ESM::ESMWriter& esm = info.writer;
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding(info.encoding));
    esm.setEncoder(&encoder);
    esm.setAuthor(*sit++);
    esm.setDescription(*sit++);
    esm.setVersion(info.data.version);
    esm.setRecordCount(info.data.mRecords.size());

    for (std::vector<ESM::Header::MasterData>::iterator it = info.data.masters.begin(); it != info.data.masters.end(); ++it)
        esm.addMaster(it->name, it->size);

    std::fstream save(info.outname.c_str(), std::fstream::out | std::fstream::binary);
    esm.save(save);

    int saved = 0;
    typedef std::deque<EsmTool::RecordBase *> Records;
    Records &records = info.data.mRecords;
    for (Records::iterator it = records.begin(); it != records.end(); ++it) {
        EsmTool::RecordBase *record = *it;
        const ESM::NAME& typeName = record->getType();
        if (typeName.toString() != *sit) {
        	printf("ERROR: Wrong Type Name at record %d (expected %s, got %s)\n",saved,typeName.toString().c_str(),sit->c_str());
        	esm.close();
        	save.close();
        	return 1;
        }

        if (record->getId() != *++sit) {
        	printf("Replacing ID '%s' with new ID '%s'\n",record->getId().c_str(),sit->c_str());
        	record->setId(*sit);
        }
        ++sit;

        esm.startRecord(typeName.toString(), record->getFlags());

        record->fromtext(sit);
        record->save(esm);

        if (typeName.intval == ESM::REC_CELL) {
            ESM::Cell *ptr = &record->cast<ESM::Cell>()->get();
            if (!info.data.mCellRefs[ptr].empty()) {
                typedef std::deque<std::pair<ESM::CellRef, bool> > RefList;
                RefList &refs = info.data.mCellRefs[ptr];
                for (RefList::iterator refIt = refs.begin(); refIt != refs.end(); ++refIt)
                {
                    refIt->first.save(esm, refIt->second);
                }
            }
        }

        esm.endRecord(typeName.toString());

        saved++;
        int perc = (int)((saved / (float)info.data.mRecords.size())*100);
        if (perc % 10 == 0)
        {
            std::cout << "\r" << perc << "%";
        }
    }

    std::cout << "\rDone!" << std::endl;

    esm.close();
    save.close();

    return 0;
}

int strings(Arguments& info)
{
    ESM::ESMReader& esm = info.reader;
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding(info.encoding));
    esm.setEncoder(&encoder);

    std::string filename = info.filename;
    esm.open(filename);

    info.data.author = esm.getAuthor();
    info.data.description = esm.getDesc();
    info.data.masters = esm.getGameFiles();
    info.storage.push_back(esm.getAuthor());
    info.storage.push_back(esm.getDesc());

    // Loop through all records
    while(esm.hasMoreRecs())
    {
        ESM::NAME n = esm.getRecName();
        uint32_t flags;
        esm.getRecHeader(flags);

        EsmTool::RecordBase *record = EsmTool::RecordBase::create(n);
        if (!record) {
            std::cerr << "ERROR: No reader for " << n.toString() << std::endl;
            return 1;
        }

        record->setFlags(static_cast<int>(flags));
        record->setPrintPlain(info.plain_given);
        record->load(esm);

        info.storage.push_back(n.toString());
        info.storage.push_back(record->getId());
        record->textdump(info.storage);

        info.data.mRecords.push_back(record);
        ++info.data.mRecordStats[n.intval];
    }

    if (info.readback.empty()) {
        //Print all info
        int num = 0;
        for (auto &i : info.storage)
            printf("$$%d %s\n",num++,i.c_str());

    } else {
        std::cout << "Read-back mode" << std::endl;
        return readback(info);
    }

    return 0;
}

int write_original_names(Arguments& mast, Arguments& plug, Arguments& out)
{
    std::cout << std::endl << "Saving records to: " << out.outname << "..." << std::endl;

    ESM::ESMWriter& esm = out.writer;
    ToUTF8::Utf8Encoder encoder (ToUTF8::calculateEncoding(out.encoding));
    esm.setEncoder(&encoder);
    esm.setAuthor(plug.data.author);
    esm.setDescription(plug.data.description);
    esm.setVersion(plug.data.version);
    esm.setRecordCount(plug.data.mRecords.size());

    for (std::vector<ESM::Header::MasterData>::iterator it = plug.data.masters.begin(); it != plug.data.masters.end(); ++it)
        esm.addMaster(it->name, it->size);

    std::fstream save(out.outname.c_str(), std::fstream::out | std::fstream::binary);
    esm.save(save);

    int saved = 0;
    typedef std::deque<EsmTool::RecordBase *> Records;
    Records &records = plug.data.mRecords;
    Records &mrecords = mast.data.mRecords;
    for (auto &r : records) {
        EsmTool::RecordBase* rec = r;
        auto org = std::find_if(mrecords.begin(),mrecords.end(),
                [rec] (EsmTool::RecordBase* a) { return a->getId() == rec->getId(); });

        if (org != mrecords.end() && (*org)->getName() != rec->getName()) {
            printf("DEBUG: replacing record name from '%s' to '%s'\n",rec->getName().c_str(),(*org)->getName().c_str());
            rec->setName((*org)->getName());
        }

        const ESM::NAME& typeName = rec->getType();
        esm.startRecord(typeName.toString(), rec->getFlags());
        rec->save(esm);

        if (typeName.intval == ESM::REC_CELL) {
            ESM::Cell *ptr = &rec->cast<ESM::Cell>()->get();
            if (!plug.data.mCellRefs[ptr].empty()) {
                typedef std::deque<std::pair<ESM::CellRef, bool> > RefList;
                RefList &refs = plug.data.mCellRefs[ptr];
                for (RefList::iterator refIt = refs.begin(); refIt != refs.end(); ++refIt)
                {
                    refIt->first.save(esm, refIt->second);
                }
            }
        }

        esm.endRecord(typeName.toString());

        saved++;
        int perc = (int)((saved / (float)plug.data.mRecords.size())*100);
        if (perc % 10 == 0)
        {
            std::cout << "\r" << perc << "%";
        }
    }

    std::cout << "\rDone!" << std::endl;

    esm.close();
    save.close();

    return 0;
}

int revert(Arguments& info)
{
    if (info.filename.empty() || info.outname.empty()) {
        std::cout << "You need to specify input and output files" << std::endl;
        return 1;
    }
    if (info.master.empty()) {
        std::cout << "You need to specify master file" << std::endl;
        return 1;
    }

    Arguments mast;
    Arguments plug;

    mast.quiet_given = true;
    plug.quiet_given = true;

    mast.raw_given = 0;
    plug.raw_given = 0;

    mast.mode = "clone";
    plug.mode = "clone";

    mast.encoding = info.encoding;
    plug.encoding = info.encoding;

    mast.filename = info.master;
    plug.filename = info.filename;

    if (load(mast) != 0)
    {
        std::cout << "Failed to load " << info.filename << ", aborting comparison." << std::endl;
        return 1;
    }

    if (load(plug) != 0)
    {
        std::cout << "Failed to load " << info.outname << ", aborting comparison." << std::endl;
        return 1;
    }

    return write_original_names(mast,plug,info); //guts
}

int main(int argc, char**argv)
{
    try
    {
        Arguments info;
        if(!parseOptions (argc, argv, info))
            return 1;

        if (info.mode == "dump")
            return load(info);
        else if (info.mode == "clone")
            return clone(info);
        else if (info.mode == "comp")
            return comp(info);
        else if (info.mode == "strings")
            return strings(info);
        else if (info.mode == "revert")
            return revert(info);
        else {
            printf("ERROR: Invalid mode %s\n",info.mode.c_str());
            return 1;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
