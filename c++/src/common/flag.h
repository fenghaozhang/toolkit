#ifndef _SRC_COMMON_FLAG_H
#define _SRC_COMMON_FLAG_H

/**This header defines all macros required to define/declare/use
 * global flag.
 *
 * [A] In the same file:
 * 1. Re-DEFINE as same type: compiling error duplicated (both on
 * FLAG_TYPE_xx, and FLAG_xx)
 * 2. Re-DEFINE as diff type: compiling error duplicated (on
 * FLAG_xx )
 * 3. DECLARE after DEFINE with a diff type: conflicted
 * 4. DECLARE after DEFINE with the same type: fine
 * 5. DECLARE one flag twice: fine
 * 6. Flag used without correct DEFINE or DECLARE: compiling error, not
 * found.
 *
 * [B] Cross file:
 * 1. DEFINE the same flag with the same type: link error
 * 2. DEFINE the same flag with the diff type: link error
 * (if declared flag is never used: no error at all)
 * 3. DECLARE with no DEFINE: link error
 * 4. DECLARE & DEFINE conflicts on the type: link error
**/

#include <map>
#include <vector>
#include <string>
#include <iostream>

#include "src/common/any.h"

using std::string;

/** Currently, we use std::string implementation for (char *). Maybe latter,
 * implementation of string will be replaced
 */

/**                 WARNING
 * ALL MACROS TO DEFINE A FLAG MUST BE USED IN A GLOBAL SCOPE,
 * NOT INSIDE ANY NAMESPACE OR CLASS OR FUNCTION BODY.
 */

/**
 * Previously defined macros are hard to remember to some extent.
 * Here defines a few substitude, and old names are obsoleted.
 * Obsolete macro names include
 * DEFINE_FLAG_FLT  (now renamed as DEFINE_FLAG_DOUBLE)
 * DEFINE_FLAG_STR  (now renamed as DEFINE_FLAG_STRING)
 * DECLARE_FLAG_FLT (now renamed as DECLARE_FLAG_DOUBLE)
 * DECLARE_FLAG_STR (now renamed as DECLARE_FLAG_STRING)
 * STRFLAG          (now renamed as STRING_FLAG)
 * INT32FLAG        (now renamed as INT32_FLAG)
 * BOOLFLAG         (now renamed as BOOL_FLAG)
 * FLTFLAG          (now renamed as DOUBLE_FLAG)
 */

/** Macro to define INT32 flag. Must be used in global scope.                */
#define DEFINE_FLAG_INT32(name, value, desc)                                 \
    namespace GlobalFlag {                                                   \
        int32_t FLAG_INT32_##name = FlagRepository::CreateFlagEx<int32_t>(   \
                    __FILE__, __LINE__,                                      \
                    std::string(#name), std::string(desc),                   \
                    &(FLAG_INT32_##name), value);                            \
        int32_t &FLAG_##name = FLAG_INT32_##name;                            \
    }

/** Macro to define INT64 flag. Must be used in global scope */
#define DEFINE_FLAG_INT64(name, value, desc)                                \
    namespace GlobalFlag                                                    \
    {                                                                       \
        int64_t FLAG_INT64_##name = FlagRepository::CreateFlagEx<int64_t>(  \
            __FILE__, __LINE__,                                             \
            std::string(#name),                                             \
            std::string(#desc),                                             \
            &(FLAG_INT64_##name),                                           \
            value);                                                         \
        int64_t &FLAG_##name = FLAG_INT64_##name;                           \
    }

/** Macro to define BOOL flag. Must be used in global scope.               */
#define DEFINE_FLAG_BOOL(name, value, desc)                                \
    namespace GlobalFlag {                                                 \
        bool FLAG_BOOL_##name = FlagRepository::CreateFlagEx<bool>(        \
                    __FILE__, __LINE__,                                    \
                    std::string(#name), std::string(desc),                 \
                    & (FLAG_BOOL_##name), value);                          \
        bool &FLAG_##name = FLAG_BOOL_##name;                              \
    }

/** Macro to define DOUBLE flag. Must be used in global scope.            */
#define DEFINE_FLAG_DOUBLE(name, value, desc)                             \
    namespace GlobalFlag {                                                \
        double FLAG_DOUBLE_##name = FlagRepository::CreateFlagEx<double>( \
            __FILE__, __LINE__,                                           \
                std::string(#name), std::string(desc),                    \
                & (FLAG_DOUBLE_##name), value);                           \
        double &FLAG_##name = FLAG_DOUBLE_##name;                         \
    }

/** Macro to define STRING flag. Must be used in global scope.            */
#define DEFINE_FLAG_STRING(name, value, desc)                             \
    namespace GlobalFlag {                                                \
        string& FLAG_STRING_##name = FlagRepository::CreateStringFlag(    \
                __FILE__, __LINE__,                                       \
                string(#name), string(desc),                              \
                value);                                                   \
        string &FLAG_##name = FLAG_STRING_##name;                         \
    }

/** Macro to declare INT32 flag                                           */
#define DECLARE_FLAG_INT32(name)                                          \
    namespace GlobalFlag {extern int32_t FLAG_INT32_##name;}

/** Macro to declare INT64 flag                                           */
#define DECLARE_FLAG_INT64(name)                                          \
    namespace GlobalFlag {extern int64_t FLAG_INT64_##name;}

/** Macro to declare BOOL flag                                            */
#define DECLARE_FLAG_BOOL(name)                                           \
    namespace GlobalFlag {extern bool FLAG_BOOL_##name;}

/** Macro to decclare STRING flag                                         */
#define DECLARE_FLAG_STRING(name)                                         \
    namespace GlobalFlag {                              \
        extern std::string& FLAG_STRING_##name;}

/** Macro to declare DOUBLE flag                                          */
#define DECLARE_FLAG_DOUBLE(name)                                         \
    namespace GlobalFlag {                              \
        extern double FLAG_DOUBLE_##name;}

/** Macro to access flags */
#define INT32_FLAG(name) (GlobalFlag::FLAG_INT32_##name)
#define INT64_FLAG(name) (GlobalFlag::FLAG_INT64_##name)
#define BOOL_FLAG(name) (GlobalFlag::FLAG_BOOL_##name)
#define DOUBLE_FLAG(name) (GlobalFlag::FLAG_DOUBLE_##name)
#define STRING_FLAG(name) (GlobalFlag::FLAG_STRING_##name)

namespace GlobalFlag
{
    enum FlagType {
        BOOL,    ///< internal stored as bool.
        DOUBLE,  ///< internal stored as double
        INT32,   ///< internal stored as int32_t
        STRING,  ///< internal stored as string
        INT64,   ///< internal stored as int64_t
    };

    struct FlagInfo
    {
        FlagType mType;
        union
        {
            bool b;
            double d;
            int32_t i;
            char* s;
            int64_t l;
        } mValue;
    };

    class FlagRepository {
    private:
        class FlagDescriptor{
        public:
            static const char *strTypename[];

            const char      *mFile;     ///< where the flag is defined
            const int       mLine;      ///< where the flag is defined
            const string    mName;      ///< flag name
            const string    mDesc;      ///< flag description
            const FlagType  mFlagType;  ///< flag type
            void *const     mpData;     ///< pointer to the flag variable
            // Mutex mMutex;
            
            /** to create a FlagDescriptor for type DOUBLE */
            FlagDescriptor(
                    const char *file,    ///< which file the flag is defined in
                    int line,            ///< at which line
                    const string &name,  ///< flag name
                    const string& desc,  ///< flag description
                    double *const v      ///< pointer to the flag variable
                    );
            /** to create a FlagDescriptor for type BOOL */
            FlagDescriptor(
                    const char * file,
                    int line,
                    const string &name,
                    const string& desc,
                    bool *const v
                    );
            /** to create a FlagDescriptor for type INT32 */
            FlagDescriptor(
                    const char * file,
                    int line,
                    const string &name, const string& desc,
                    int32_t *const v
                    );
            /** to create a FlagDescriptor for type INT64 */
            FlagDescriptor(
                    const char * file,
                    int line,
                    const string &name, const string& desc,
                    int64_t *const v
                    );
            /** to create a FlagDescriptor for type STRING */
            FlagDescriptor(
                    const char * file,
                    int line,
                    const string &name, const string& desc,
                    std::string *const v
                    );
        }; // end of class FlagDescriptor
        typedef std::map<string, FlagDescriptor> FlagMapType;

#if 0 // these two flags are moved into CreateFlag as a static member
        static FlagMapType mAllFlags;///< map from name to FlagDescriptor
        static Mutex mMutex; ///< mutex for synchronization at flag creation
#else

        friend class FlagSaverImpl;

        struct CoreData
        {
            FlagMapType mAllFlags;///< map from name to FlagDescriptor
            // Mutex mMutex;         ///< mutex for synchronization at flag creation
        };
        static CoreData& GetCoreData();
#endif


        /** Get a flag usign a wrong type */
        static void RuntimeTypeError(
                const char *usefile,
                const int useline,
                const FlagType usedAs,
                const FlagDescriptor& fd
                );

        /** Get a flag not existing */
        static void RuntimeNotFoundError(
                const char *usefile,
                const int useline,
                const FlagType usedAs,
                const std::string& name
                );
        /** Create a flag already existing */
        static void RuntimeExistError(
                const char *file,
                const int line,
                const FlagType usedAs,
                const FlagDescriptor& fd
                );

        static const int DUPLICATE_REGISTRATION = -100;
        /** return non-zero if fail */
        static int CreateFlag(const FlagDescriptor& fd);

        static std::string& mCommandLineParams;
        static std::string& getCommandLineParams(); 
    public:
        /** iterator */
        static inline FlagMapType::iterator GetFlagIterator()
        {
            return GetCoreData().mAllFlags.begin();
        }
        static inline FlagMapType::iterator GetFlagIterator(const string& name)
        {
            return GetCoreData().mAllFlags.find(name);
        }
        static inline FlagMapType::iterator end()
        {
            return GetCoreData().mAllFlags.end();
        }

        // static void SetFlag(const apsara::json::JsonMap& flags);
        /**
         * To access flags. It is suggested to use the DECLARE and
         * FLAG macro instead of these methods directly. They may
         * cause runtime error such as using GetFlagBool to get a int32
         * flag, or get a flag not existing.
         */
        static void SetFlag(
                const string& name,
                /** The value is parsed according to the flag type*/
                const string& value
                );
        /**
         * If can not find flag, returns false.
         */
        static bool SetFlag2(
                const string& name,
                /** The value is parsed according to the flag type*/
                const string& value
                );
        static void SetFlag(const FlagDescriptor& descriptor, const FlagInfo& info);

        static string GetFlag(const string& name);
        static bool HasFlag(const string& name);

        /** show all flags grouped by the defining file, with subscription of each flag */
        static void DumpAll(std::ostream& os);
        /** dump all flags as a json map */
        // static apsara::Any DumpAsJson();
        static void Dump(std::ostream& os);

        /** dump all flags as a command line parameter (in the format, --key1=v1 --key2=v2)
         * @param all unset only dump parameters given on the command line
	 */
        static std::string Dump(bool all = false);

        /**
         * Tool for parsing command-line
         * --flag_name1[=value1] --flag_name2[=value2]
         *  If value is omiitted, default value is used:
         *  0 for int32, int64 and double, true for bool, and empty string for string)
         */
        static int parse(
                const int argc,
                char **argv,
                /** Whether continuing parse if parse error occurs */
                const bool resumeAfterFailure
                );
        static int Parse(
                const int argc,
                char **argv,
                const bool resumeAfterFailure)
        {
            return parse(argc, argv, resumeAfterFailure);
        }
        static int ParseAndRemove(
                int& argc,
                char **argv,
                const bool resumeAfterFailure);

        /** DEFINE macro uses this to register flag in the mAllFlags */
        template<typename T>
        static T& CreateFlagEx(
                const char *file,   ///< File the flag is defined
                const int lineno,   ///< Line where the flag is defined
                const string& name, ///< flag name
                const string& desc, ///< flag descriptor
                T* p,               ///< pointer to the flag variable
                T v                 ///< flag itself. T& v may fail on const!
                )
        {
            FlagDescriptor fd(file, lineno, name, desc, p);
            int ret = CreateFlag(fd);
            switch (ret)
            {
                case 0:
                    *p = v;
                    return *p;
                case DUPLICATE_REGISTRATION:
                    return *p;
                default:
                    perror("Create flag fail");
                    throw ret;  /// program never goes here
            }
        }

        static std::string& CreateStringFlag(
                const char *file,
                const int lineno,
                const string& name,
                const string& desc,
                const string& value);

    }; // end of class FlagRepository

    // Save the flags from flag repository to flag saver.
    // It saves all the global flags registered at construct time,
    // and restores it at destroy time.
    // It's NOT a thread-safe class.
    class FlagSaverImpl
    {
    public:
        struct FlagData
        {
            FlagRepository::FlagDescriptor* mDescriptor;
            FlagInfo mInfo;
        };

        ~FlagSaverImpl();

        void SaveFlags();

        void RestoreFlags();

        bool Compare(const FlagSaverImpl* rhs, std::vector<std::pair<std::string, FlagInfo[2]> >& diffs);

    private:
        bool FlagValueCompare(FlagInfo lhs, FlagInfo rhs);

        bool PushDiffIfNeed(FlagData* lhs, FlagData* rhs, std::vector<std::pair<std::string, FlagInfo[2]> >& diffs);

        // We do NOT need to save flags in std::map. Because the iterator of vector
        // is more faster.
        std::vector<FlagData*> mSavedFlags;
    };

    class FlagSaver
    {
    public:
        FlagSaver()
            : mSaverImpl(new FlagSaverImpl())
        {
            mSaverImpl->SaveFlags();
        }

        ~FlagSaver()
        {
            mSaverImpl->RestoreFlags();
            delete mSaverImpl;
        }

        // Make a snapshot.
        void Save()
        {
            delete mSaverImpl;
            mSaverImpl = new FlagSaverImpl();
            mSaverImpl->SaveFlags();
        }

        bool Compare(const FlagSaver& rhs, std::vector<std::pair<std::string, FlagInfo[2]> >& diffs)
        {
            return mSaverImpl->Compare(rhs.mSaverImpl, diffs);
        }

    private:
        FlagSaverImpl* mSaverImpl;
    };

}   // GlobalFlag

#endif // _SRC_COMMON_FLAG_H
