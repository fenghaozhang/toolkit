#include "src/base/env.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/resource.h>
#include <sys/types.h>

#include "src/common/errorcode.h"
#include "src/string/string_util.h"

Env* Env::sInstance = NULL;

void Env::Reset(Env* env)
{
    Env::sInstance = env;
}

Env* Env::Instance()
{
    if (Env::sInstance == NULL)
    {
        static Env env;
        Env::sInstance = &env;
    }
    return Env::sInstance;
}

Status Env::GetHostName(std::string* hostname)
{
    char buf[1024];
    int ret = gethostname(buf, 1023);
    if (UNLIKELY(ret != 0))
    {
        return INTERNAL_ERROR;
    }
    *hostname = std::string(buf);
    return OK;
}

std::map<std::string, std::string> Env::GetEnvMap()
{
    const char* const* x = environ;
    std::map<std::string, std::string> envs;
    while (*x)
    {
        const char *p = strstr(*x, "=");
        std::string key, value;
        if (p == 0)
        {
            key = *x;
        }
        else
        {
            key = std::string(*x, p-*x);
            value = std::string(p+1);
        }
        envs.insert(std::make_pair(key, value));
        ++x;
    }
    return envs;
}

Status Env::GetEnv(const std::string& key, std::string* value)
{
    char *v = getenv(key.c_str());
    if (UNLIKELY(!v))
    {
        return INTERNAL_ERROR;
    }
    *value = std::string(v);
    return OK;
}

Status Env::RemoveEnv(const std::string& key)
{
    if (UNLIKELY(unsetenv(key.c_str())))
    {
        if (errno == EINVAL)
        {
            return INVALID_PARAMETER;
        }
        else
        {
            return INTERNAL_ERROR;
        }
    }
    return OK;
}

Status Env::SetEnv(const std::string& key,
                   const std::string& value,
                   bool overwrite)
{
    if (UNLIKELY(setenv(key.c_str(), value.c_str(), overwrite ? 1 : 0)))
    {
        if (errno == EINVAL)
        {
            return INVALID_PARAMETER;
        }
        else
        {
            return INTERNAL_ERROR;
        }
    }
    return OK;
}

Status Env::PrependEnv(const std::string& key,
                       const std::string& value,
                       const std::string& separator)
{
    std::string final;
    Status status = Env::GetEnv(key, &final);
    if (!status.IsOk())
    {
        return status;
    }
    if (value.size() > 0 && final.size() > 0)
    {
        final = value + separator + final;
    }
    return Env::SetEnv(key, final, true);
}

Status Env::AppendEnv(const std::string& key, const std::string& value,
                      const std::string& separator)
{
    std::string final;
    Status status = GetEnv(key, &final);
    if (!status.IsOk())
    {
        return status;
    }
    if (value.size() > 0 && final.size() > 0)
    {
        final += separator + value;
    }
    return SetEnv(key, final, true);
}

Status Env::SetLimit(const std::string& key, const std::string& value)
{
    int res = getLimitResourceKey(key);
    if (res == INVALID_PARAMETER)
    {
        return res;
    }
    struct rlimit rlim, rlimNew;
    if (value == "unlimited")
    {
        rlimNew.rlim_cur = RLIM_INFINITY;
    }
    else
    {
        rlimNew.rlim_cur = StringToLongLong(value);
    }

    if (UNLIKELY(getrlimit(res, &rlim) != 0))
    {
        return INTERNAL_ERROR;
    }
    rlimNew.rlim_max = rlimNew.rlim_cur > rlim.rlim_max ?
        rlimNew.rlim_cur : rlim.rlim_max;
    if (UNLIKELY(setrlimit(res, &rlimNew) != 0))
    {
        return INTERNAL_ERROR;
    }
    return OK;
}

Status Env::GetLimit(const std::string& key, std::string* value)
{
    int res = getLimitResourceKey(key);
    if (res == INVALID_PARAMETER)
    {
        return res;
    }

    struct rlimit rlim;
    if (UNLIKELY(getrlimit(res, &rlim) != 0))
    {
        return INTERNAL_ERROR;
    }
    if (rlim.rlim_cur == RLIM_INFINITY)
    {
        *value = "unlimited";
    }
    else
    {
        *value = Uint64ToString(rlim.rlim_cur);
    }
    return OK;
}

std::string Env::GetSelfPath()
{
    std::string path;
    char buf[PATH_MAX + 1];

    pid_t pid = getpid();

    if (0 > snprintf(buf, PATH_MAX, "%d", pid))
    {
        return path;
    }
    std::string pidExePath = "/proc/";
    for (int i = 0; i < PATH_MAX; ++i)
    {
        if ('\0' == buf[i])
        {
            break;
        }
        pidExePath += buf[i];
    }
    pidExePath += "/exe";

    ssize_t size = readlink(pidExePath.c_str(), buf, PATH_MAX);
    if (-1 == size)
    {
        return path;
    }
    buf[size] = '\0';
    path = buf;

    return path;
}

std::string Env::GetSelfDir()
{
    std::string path = GetSelfPath();
    return path.substr(0, path.rfind('/') + 1);
}

std::string Env::GetSelfName()
{
    std::string path = GetSelfPath();
    return path.substr(path.rfind('/') + 1);
}

int Env::getLimitResourceKey(const std::string& key)
{
    int res = -1;

    if (key == std::string("core"))
    {
        res = RLIMIT_CORE;
    }
    else if (key == std::string("data"))
    {
        res = RLIMIT_DATA;
    }
    else if (key == std::string("fsize"))
    {
        res = RLIMIT_FSIZE;
    }
    else if (key == std::string("nofile"))
    {
        res = RLIMIT_NOFILE;
    }
    else if (key == std::string("stack"))
    {
        res = RLIMIT_STACK;
    }
    else if (key == std::string("cpu"))
    {
        res = RLIMIT_CPU;
    }
    else if (key == std::string("as"))
    {
        res = RLIMIT_AS;
    }
    else
    {
        return INVALID_PARAMETER;
    }
    return res;
}
