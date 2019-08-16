#include <map>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <ctype.h>

#include "src/common/flag.h"

using std::string;
using std::cout;
using std::cerr;
using std::endl;

/* class FlagRepository::FlagDescriptor */
namespace GlobalFlag
{
    // TODO: This method could be entirely updated as a call to
    // proper method in string_tools when it is ready.
    static bool parseBool(const string &in)
    {
        if (in.size() == 0) return true;
        string lower = in;
        //TODO: updated it with a tool in apsara::string_tools
        for(string::iterator it = lower.begin(); it != lower.end(); ++it)
        {
            *it = tolower(*it);
        }
        if (lower == "true") return true;
        if (lower == "false") return false;
        int n = atoi(in.c_str());
        return n!=0;
    }
    /* to convert from FlagType to readable string */
    const char* FlagRepository::FlagDescriptor::strTypename[] =
    {
        "BOOL",
        "DOUBLE",
        "INT32",
        "STRING",
        "INT64"
    };

    /** to create a FlagDescriptor for type DOUBLE */
    FlagRepository::FlagDescriptor::FlagDescriptor(
            const char *file,
            int line,
            const string& name,
            const string& desc,
            double *const v):
        mFile(file), mLine(line),
        mName(name), mDesc(desc),
        mFlagType(DOUBLE),
        mpData(v)
    {
    }

    /** to create a FlagDescriptor for type BOOL */
    FlagRepository::FlagDescriptor::FlagDescriptor(
            const char *file,
            int line,
            const string& name,
            const string& desc,
            bool *const v):
        mFile(file), mLine(line),
        mName(name), mDesc(desc),
        mFlagType(BOOL),
        mpData(v)
    {
    }

     /** to create a FlagDescriptor for type INT32 */
     FlagRepository::FlagDescriptor::FlagDescriptor(
            const char *file,
            int line,
            const string& name,
            const string& desc,
            int32_t *const v):
        mFile(file), mLine(line),
        mName(name), mDesc(desc),
        mFlagType(INT32),
        mpData(v)
    {
    }

     /** to create a FlagDescriptor for type INT64 */
     FlagRepository::FlagDescriptor::FlagDescriptor(
            const char *file,
            int line,
            const string& name,
            const string& desc,
            int64_t *const v):
        mFile(file), mLine(line),
        mName(name), mDesc(desc),
        mFlagType(INT64),
        mpData(v)
    {
    }

    /* to create a FlagDescriptor for type STRING */
    FlagRepository::FlagDescriptor::FlagDescriptor(
            const char *file,
            int line,
            const string& name,
            const string& desc,
            string *const v):
        mFile(file), mLine(line),
        mName(name), mDesc(desc),
        mFlagType(STRING),
        mpData(v)
    {
    }
#if 0 // see comments in flag.h
    /* class FlagRepository */
    /** static member, to store the details of all flags */
    FlagRepository::FlagMapType FlagRepository::mAllFlags;

    /** to make synchronization at creating new flag */
    Mutex FlagRepository::mMutex;
#endif
    std::string& FlagRepository::mCommandLineParams = FlagRepository::getCommandLineParams();

    /* private */
    std::string& FlagRepository::getCommandLineParams()
    {
        static std::string *cmd = new std::string();
        return *cmd;
    }

    FlagRepository::CoreData& FlagRepository::GetCoreData()
    {
        static CoreData coreData;
        return coreData;
    }

    std::string& FlagRepository::CreateStringFlag(
                const char *file,
                const int lineno,
                const string& name,
                const string& desc,
                const string& value)
    {
        ScopedLock lock(GetCoreData().mMutex);
        FlagMapType::iterator it = GetFlagIterator(name);
        if (it == GetCoreData().mAllFlags.end())
        {
            std::string* data = new std::string(value);
            FlagDescriptor fd(file, lineno, name, desc, data);
            it = GetCoreData().mAllFlags.insert(FlagMapType::value_type(name, fd)).first;
        }
        return *static_cast<std::string*>(it->second.mpData);
    }
 
    int FlagRepository::CreateFlag(const FlagDescriptor& fd)
    {
        ScopedLock lock(GetCoreData().mMutex);

        const string& name = fd.mName;
        FlagMapType::iterator it = GetFlagIterator(name);
        if (it == GetCoreData().mAllFlags.end()) {
            GetCoreData().mAllFlags.insert(FlagMapType::value_type(name,fd));
            return 0;
        }
        if (it->second.mpData == fd.mpData)
        {
            return DUPLICATE_REGISTRATION;
        }

        throw std::logic_error(
                "Duplicate creation. "
                "Should be detected by compiler.");
        return -1;
    }

    void FlagRepository::SetFlag(const apsara::json::JsonMap& flags)
    {
        for (decltype(flags.begin()) it = flags.begin();
             it != flags.end();
             ++it)
        {
            const std::string& flagName = it->first;
            FlagMapType::iterator it2 = GetFlagIterator(flagName);
            if (it2 == end())
            {
                continue;
            }
            if (flagName == "production" && apsara::security::SecurityFlag::HasFreeze())
            {
                continue;
            }
            else if (flagName == "disable_validator" && apsara::security::SecurityFlag::GetFreeze())
            {
                continue;
            }
            FlagDescriptor& fd = it2->second;
            ScopedLock lock(fd.mMutex);
            switch (fd.mFlagType) {
            case DOUBLE:
                (*static_cast<double *>(fd.mpData)) = apsara::json::JsonNumberCast<double>(it->second);
                break;
            case INT32:
                 (*static_cast<int32_t *>(fd.mpData))  =
                         apsara::json::JsonNumberCast<int32_t>(it->second);
                 break;
            case INT64:
                 (*static_cast<int64_t *>(fd.mpData))  =
                         apsara::json::JsonNumberCast<int64_t>(it->second);
                 break;
            case STRING:
                 (*static_cast<string *>(fd.mpData)) = apsara::AnyCast<std::string>(it->second);
                 break;
            case BOOL:
                 (*static_cast<bool *>(fd.mpData)) = apsara::AnyCast<bool>(it->second);
                 break;
            default:
                throw std::runtime_error("bad flag type in memory");
            }
        }
    }

    /** set the value for a flag. The value is given as plain text and
     * this method will choose  the proper convertion way according to
     * its defined type
     */
    void FlagRepository::SetFlag(const string& name, const string& value)
    {

        bool rc = SetFlag2(name, value);
        if (!rc)
        {
            throw std::invalid_argument("flag not found");
        }
        return;
    }

    /** Set flag value, if flag is not found, return false instead of throw
     *  exception.
     */
    bool FlagRepository::SetFlag2(const string& name, const string& value)
    {
        FlagMapType::iterator it = GetFlagIterator(name);
        if (it == end())
            return false;
        if (name == "production" && apsara::security::SecurityFlag::HasFreeze())
        {
            throw std::runtime_error("Flag 'production' has already been set");
        }
        else if (name == "disable_validator" && apsara::security::SecurityFlag::GetFreeze())
        {
            throw std::runtime_error("Flag 'disable_validator' has been freezed");
        }
        FlagDescriptor& fd = it->second;
        ScopedLock lock(fd.mMutex);
        switch (fd.mFlagType) {
        case DOUBLE:
            (*static_cast<double *>(fd.mpData)) = StringTo<double>(value);
            break;
        case INT32:
            (*static_cast<int32_t *>(fd.mpData))  = StringTo<int32_t>(value);
            break;
        case INT64:
            (*static_cast<int64_t *>(fd.mpData))  = StringTo<int64_t>(value);
            break;
        case STRING:
            (*static_cast<string *>(fd.mpData)) = value;
            break;
        case BOOL:
            (*static_cast<bool *>(fd.mpData)) =
                static_cast<bool>(parseBool(value.c_str()));
            break;
        default:
            throw std::runtime_error("bad flag type in memory");
        }

        mCommandLineParams += " --" + name;
        if (fd.mFlagType != BOOL || value.size() > 0)
        {
            mCommandLineParams += "=" + value;
        }
        return true;
    }

    void FlagRepository::SetFlag(const FlagDescriptor& descriptor, const FlagInfo& info)
    {
        FlagMapType::iterator it = GetFlagIterator(descriptor.mName);
        if (it == end())
        {
            return; // Got removed.
        }
        FlagDescriptor& fd = it->second;
        ScopedLock lock(fd.mMutex);
        if (fd.mFlagType != info.mType)
        {
            return; // Ignore
        }
        switch (fd.mFlagType)
        {
        case DOUBLE: *static_cast<double *>(fd.mpData) = info.mValue.d;       break;
        case INT32:  *static_cast<int32_t *>(fd.mpData) = info.mValue.i;      break;
        case INT64:  *static_cast<int64_t *>(fd.mpData) = info.mValue.l;      break;
        case STRING:
        {
            if ((static_cast<std::string *>(fd.mpData))->compare(info.mValue.s) != 0)
            {
                *static_cast<std::string *>(fd.mpData) = info.mValue.s;
            }
            break;
        }
        case BOOL:   *static_cast<bool *>(fd.mpData) = info.mValue.b;         break;
        }
    }

    bool FlagRepository::HasFlag(const string& name)
    {
        FlagMapType::iterator it = GetFlagIterator(name);
        return (it != end());
    }
    string FlagRepository::GetFlag(const string& name)
    {
        char buffer[100]; size_t used;
        FlagMapType::iterator it = GetFlagIterator(name);
        if (it == end()) throw std::invalid_argument("flag not found");
        FlagDescriptor& fd = it->second;
        ScopedLock lock(fd.mMutex);
        switch (fd.mFlagType)
        {
            case DOUBLE:
                used = snprintf(
                        buffer,
                        sizeof(buffer),
                        "%g",
                        *static_cast<double *>(fd.mpData));
                return string(buffer,used);
            case INT32:
                used = snprintf(
                        buffer,
                        sizeof(buffer),
                        "%d",
                        *static_cast<int *>(fd.mpData));
                return string(buffer,used);
            case INT64:
                return ToString(*static_cast<int64_t *>(fd.mpData));
            case STRING:
                return *static_cast<string *>(fd.mpData);
                break;
            case BOOL:
                return (*static_cast<bool *>(fd.mpData)) ?
                    "TRUE" : "FALSE";
            default:
                throw std::invalid_argument("flag not found");
        }
        return "";
    }

    /** parse command line argments to set flag
     * it returns -1 if resume is not set and an parsing error occurs
     * it returns 0 if no error found or resume is set
     */
    int FlagRepository::parse(
            const int argc, char **argv,
            const bool resumeAfterFailure
            )
    {
        for (int i = 0; i < argc; ++i)
        {
            string u(argv[i]);
            /*
             * if (u == "--buildinfo")
             * {
             *     DumpBuildInfo(std::cout);
             *     exit(0);
             * }
             */
            if (u == "--flaghelp")
            {
                DumpAll(std::cout);
                exit(0);
            }
            int prefixed = (u.size() > 2 && u[0] == '-' && u[1] == '-');
            if (!prefixed)
            {
                if (!resumeAfterFailure) return -1;
                continue;
            }
            int pos;
            try {
                if (u.find("=") == string::npos)
                {
                    SetFlag(u.substr(2), "");
                } else {
                    pos = u.find("=");
                    SetFlag(u.substr(2, pos - 2), u.substr(pos + 1));
                }
            } catch (...)
            {
                if (!resumeAfterFailure) return -2;
            }
        }
        return 0;
    }

    /* parse options and remove it from argc, argv if it is recognized */
    int FlagRepository::ParseAndRemove(
            int& argc, char **argv,
            const bool resumeAfterFailure
            )
    {
        /* keep argc in another var since argc may be altered in the loop. */
        int argsize = argc;
        for (int i = 0; i < argc; ++i)
        {
            string u(argv[i]);
            /*
             * if (u == "--buildinfo")
             * {
             *     DumpBuildInfo(std::cout);
             *     exit(0);
             * }
             */
            if (u == "--flaghelp")
            {
                DumpAll(std::cout);
                exit(0);
            }
            int prefixed = (u.size() > 2 && u[0] == '-' && u[1] == '-');
            if (!prefixed)
            {
                if (!resumeAfterFailure) return -1;
                continue;
            }
            int pos;
            try {
                bool rc = false;
                if (u.find("=") == string::npos)
                {
                    rc = SetFlag2(u.substr(2), "");
                } else {
                    pos = u.find("=");
                    rc = SetFlag2(u.substr(2, pos - 2), u.substr(pos + 1));
                }

                if (rc)
                {
                    /* try to remove the global flag from arg list*/
                    for (int j = i; j < argsize - 1; j++)
                    {
                        argv[j] = argv[j + 1];
                    }
                    argc--, i--;
                }
            } catch (...)
            {
                if (!resumeAfterFailure) return -2;
            }
        }
        return 0;
    }

    apsara::Any FlagRepository::DumpAsJson()
    {
        apsara::json::JsonMap map;

        std::map<std::string, std::vector<FlagMapType::iterator> > arranged;
        for (FlagMapType::iterator it = GetFlagIterator(); it != end(); ++it)
        {
            const FlagDescriptor& fd = it->second;
            if (fd.mName == "buildinfo")
            {
                continue;
            }
            if (fd.mName == "flaghelp")
            {
                continue;
            }
            switch (fd.mFlagType)
            {
                case BOOL:
                    map[fd.mName] = *static_cast<bool *>(fd.mpData);
                    break;
                case STRING:
                    map[fd.mName] = *static_cast<std::string *>(fd.mpData);
                    break;
                case INT32:
                    map[fd.mName] = *static_cast<int32_t *>(fd.mpData);
                    break;
                case INT64:
                    map[fd.mName] = *static_cast<int64_t *>(fd.mpData);
                    break;
                case DOUBLE:
                    map[fd.mName] = *static_cast<double *>(fd.mpData);
                    break;
            }
        }

        return map;
    }

    std::string FlagRepository::Dump(bool all)
    {
        std::string flags;
        if (all)
        {
            std::map<std::string, std::vector<FlagMapType::iterator> > arranged;
            for (FlagMapType::iterator it = GetFlagIterator(); it != end(); ++it)
            {
                const FlagDescriptor& fd = it->second;
                if (fd.mName == "buildinfo")
                {
                    continue;
                }
                if (fd.mName == "flaghelp")
                {
                    continue;
                }
                flags += " --" + fd.mName + "=";
                switch (fd.mFlagType)
                {
                    case BOOL:
                        flags += *static_cast<bool *>(fd.mpData) ? "true" : "false";
                        break;
                    case STRING:
                        flags += *static_cast<std::string *>(fd.mpData);
                        break;
                    case INT32:
                        flags += apsara::ToString(*static_cast<int32_t *>(fd.mpData));
                        break;
                    case INT64:
                        flags += apsara::ToString(*static_cast<int64_t *>(fd.mpData));
                        break;
                    case DOUBLE:
                        flags += apsara::ToString(*static_cast<double *>(fd.mpData));
                        break;
                }
            }
        }
        else
        {
            flags = mCommandLineParams;
        }

        return flags.size() > 0 ? flags.substr(1) : flags;
    }

    void FlagRepository::Dump(std::ostream& os)
    {
        os << apsara::json::ToString(DumpAsJson()) << std::endl;
    }

    /* print all defined flags */
    void FlagRepository::DumpAll(std::ostream& os)
    {
        std::map<std::string, std::vector<FlagMapType::iterator> > arranged;
        for (FlagMapType::iterator it = GetFlagIterator(); it != end(); ++it)
        {
            const FlagDescriptor& fd = it->second;
            arranged[fd.mFile].push_back(it);
        }

        for (decltype(arranged.begin()) it = arranged.begin();
             it != arranged.end();
             ++it)
        {
            os << "Flags defined in " << it->first << "(" << it->second.size() << "):" << std::endl;
            for (decltype(it->second.begin()) it2 = it->second.begin();
                 it2 != it->second.end();
                 ++it2)
            {
                const FlagDescriptor& fd = (*it2)->second;

                os.width(5);
                os << fd.mLine << ": " << fd.mName << " - " << fd.mDesc << std::endl;
                os << "       ";
                switch (fd.mFlagType)
                {
                    case BOOL:
                        os << "(bool)" << ToString(*static_cast<bool *>(fd.mpData));
                        break;
                    case DOUBLE:
                        os << "(double)" << ToString(*static_cast<double *>(fd.mpData));
                        break;
                    case INT32:
                        os << "(int32)" << ToString(*static_cast<int32_t *>(fd.mpData));
                        break;
                    case INT64:
                        os << "(int64)" << ToString(*static_cast<int64_t *>(fd.mpData));
                        break;
                    case STRING:
                        os << "(string)" << ToString(*static_cast<std::string*>(fd.mpData));
                        break;
                    default:
                        os << "(unknown)0x" << fd.mpData;
                }
                os << std::endl;
            }
        }
    }

    FlagSaverImpl::~FlagSaverImpl()
    {
        for (std::vector<FlagData*>::const_iterator i = mSavedFlags.begin();
             i != mSavedFlags.end();
             ++i)
        {
            if ((*i)->mDescriptor->mFlagType == STRING)
            {
                delete[] (*i)->mInfo.mValue.s;
            }
            delete (*i)->mDescriptor;
            delete *i;
        }
    }

    void FlagSaverImpl::SaveFlags()
    {
        // Notice: CreateFlag of the repository might be conflict from flag saver when
        // the mutex is held.
        ScopedLock lock(FlagRepository::GetCoreData().mMutex);
        for (FlagRepository::FlagMapType::const_iterator i = FlagRepository::GetFlagIterator();
             i != FlagRepository::end();
             ++i)
        {
            const FlagRepository::FlagDescriptor& f = i->second;
            FlagData* flag = new FlagData;
            // Store real flag in mValue.
            switch (f.mFlagType)
            {
            case BOOL: flag->mDescriptor = new FlagRepository::FlagDescriptor(
                    f.mFile, f.mLine, f.mName, f.mDesc, static_cast<bool *>(f.mpData));
                    flag->mInfo.mType = BOOL;
                    flag->mInfo.mValue.b = *(static_cast<bool *>(f.mpData));
                    break;
            case INT32: flag->mDescriptor = new FlagRepository::FlagDescriptor(
                    f.mFile, f.mLine, f.mName, f.mDesc, static_cast<int32_t *>(f.mpData));
                    flag->mInfo.mType = INT32;
                    flag->mInfo.mValue.i = *(static_cast<int32_t *>(f.mpData));
                    break;
            case INT64: flag->mDescriptor = new FlagRepository::FlagDescriptor(
                    f.mFile, f.mLine, f.mName, f.mDesc, static_cast<int64_t *>(f.mpData));
                    flag->mInfo.mType = INT64;
                    flag->mInfo.mValue.l = *(static_cast<int64_t *>(f.mpData));
                    break;
            case DOUBLE: flag->mDescriptor = new FlagRepository::FlagDescriptor(
                    f.mFile, f.mLine, f.mName, f.mDesc, static_cast<double *>(f.mpData));
                    flag->mInfo.mType = DOUBLE;
                    flag->mInfo.mValue.d = *(static_cast<double *>(f.mpData));
                    break;
            case STRING: flag->mDescriptor = new FlagRepository::FlagDescriptor(
                    f.mFile, f.mLine, f.mName, f.mDesc, static_cast<std::string *>(f.mpData));
                    flag->mInfo.mType = STRING;
                    flag->mInfo.mValue.s = new char[(static_cast<std::string *>(f.mpData))->size() + 1];
                    strcpy(flag->mInfo.mValue.s, (static_cast<std::string *>(f.mpData))->c_str());
                    break;
            }
            mSavedFlags.push_back(flag);
        }
    }

    void FlagSaverImpl::RestoreFlags()
    {
        ScopedLock lock(FlagRepository::GetCoreData().mMutex);
        for (std::vector<FlagData*>::const_iterator i = mSavedFlags.begin();
             i != mSavedFlags.end();
             ++i)
        {
            FlagRepository::SetFlag(*((*i)->mDescriptor), (*i)->mInfo);
        }
    }

    bool FlagSaverImpl::FlagValueCompare(FlagInfo lhs, FlagInfo rhs)
    {
        if (lhs.mType != rhs.mType)
        {
            return false;
        }
        switch (lhs.mType)
        {
            case BOOL:   return lhs.mValue.b == rhs.mValue.b;
            case INT32:  return lhs.mValue.i == rhs.mValue.i;
            case INT64:  return lhs.mValue.l == rhs.mValue.l;
            case DOUBLE: return lhs.mValue.d == rhs.mValue.d;
            case STRING: return strcmp(lhs.mValue.s, rhs.mValue.s) == 0;
        }
        return true;
    }

    bool FlagSaverImpl::PushDiffIfNeed(FlagData* lhs, FlagData* rhs,
        std::vector<std::pair<std::string, FlagInfo[2]> >& diffs)
    {
        if (lhs->mDescriptor->mFlagType == rhs->mDescriptor->mFlagType &&
            !FlagValueCompare(lhs->mInfo, rhs->mInfo))
        {
            diffs.push_back(std::pair<std::string, FlagInfo[2]>());
            diffs.back().first = lhs->mDescriptor->mName;
            FlagInfo (&flagInfos)[2] = diffs.back().second;
            flagInfos[0] = lhs->mInfo;
            flagInfos[1] = rhs->mInfo;
            if (lhs->mDescriptor->mFlagType == STRING)
            {
                // Dup a string copy. It's owner's responsibility to release it.
                flagInfos[0].mValue.s = new char[strlen(lhs->mInfo.mValue.s) + 1];
                strcpy(flagInfos[0].mValue.s, lhs->mInfo.mValue.s);
                flagInfos[1].mValue.s = new char[strlen(rhs->mInfo.mValue.s) + 1];
                strcpy(flagInfos[1].mValue.s, rhs->mInfo.mValue.s);
            }
            return true;
        }
        return false;
    }

    bool FlagSaverImpl::Compare(
        const FlagSaverImpl* rhs,
        std::vector<std::pair<std::string, FlagInfo[2]> >& diffs)
    {
        diffs.clear();
        std::vector<FlagData*>::const_iterator i = mSavedFlags.begin();
        std::vector<FlagData*>::const_iterator j = rhs->mSavedFlags.begin();
        bool ret = true;
        while (i != mSavedFlags.end() && j != rhs->mSavedFlags.end())
        {
            int cmp = (*i)->mDescriptor->mName.compare((*j)->mDescriptor->mName);
            if (cmp < 0)
            {
                ++i;
            }
            else if (cmp > 0)
            {
                ++j;
            }
            else
            {
                if (PushDiffIfNeed(*i, *j, diffs))
                {
                    ret = false;
                }
                ++i;
                ++j;
            }
        }
        return ret;
    }

#if 0 // replaced with new implemetation above
    void FlagRepository::Dump(std::ostream& os)
    {
        os.width(15);
        os<<"Flag";
        os.width(8);
        os<<"Type";
        os.width(25);
        os<<"Value";

        os<<" Location_And_Desc";
        os << endl;
        for (FlagMapType::iterator it = GetFlagIterator(); it != end(); ++it)
        {
            const FlagDescriptor& fd = it->second;
            os.width(15);
            os << fd.mName;
            switch(fd.mFlagType) {
            case BOOL:
                os.width(8);
                os << "BOOL";
                os.fill('.');
                os.width(25);
                os << ToString(*static_cast<bool *>(fd.mpData));
                os.fill(' ');
                break;
            case DOUBLE:
                os.width(8);
                os << "DOUBLE";
                os.fill('.');
                os.width(25);
                os << *static_cast<double *>(fd.mpData);
                os.fill(' ');
                break;
            case INT32:
                os.width(8);
                os << "INT32";
                os.width(25);
                os.fill('.');
                os << *static_cast<int *>(fd.mpData);
                os.fill(' ');
                break;
            case STRING:
                os.width(8);
                os << "STRING";
                os.fill('.');
                os.width(25);
                os << *static_cast<string *>(fd.mpData);
                os.fill(' ');
                break;
            default:
                os.width(8);
                os << "UNKNOWN";
                os.fill('.');
                os.width(25);
                os << fd.mpData;
                os.fill(' ');
                break;
            }
            os << " " << fd.mFile
                << "[" << fd.mLine << ']'
                << ":" << fd.mDesc << endl;
        } // end for: each flag
    } // end of Dump(..)
#endif

} // namespace GlobalFlag
