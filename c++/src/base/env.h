#ifndef _SRC_BASE_ENV_H
#define _SRC_BASE_ENV_H

#include <stdint.h>
#include <sys/resource.h>
#include <sys/types.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "src/base/status.h"
#include "src/common/macros.h"

class Env
{
public:
    /**
     * Get current machine's host name
     */
    virtual Status GetHostName(std::string* hostname);

    /**
     * Get all env name and their value
     */
    virtual std::map<std::string, std::string> GetEnvMap();

    /**
     * Get the value of the specified env.
     * Return INTERNAL_ERROR if env name is bad or not found.
     */
    virtual Status GetEnv(const std::string& key, std::string* value);

    /**
     * Remove the specified env from the environ.
     * Return INVALID_PARAMETER if env name is invalid.
     */
    virtual Status RemoveEnv(const std::string& key);

    /**
     * Set value of an env. If overwrite is unset
     * and the env already exists, the env leaves unchanged.
     * Return INVALID_PARAMETER if the env name is invalid.
     * Return INTERNAL_ERROR for other errors.
     */
    virtual Status SetEnv(const std::string& key,
                          const std::string& value,
                          bool overwrite = true);

    /**
     * Prefix append env with value and separator (optionally).
     * If value or env value is empty string, or env has not been defined yet,
     * separator will not be prepended.
     */
    virtual Status PrependEnv(const std::string& key,
                              const std::string& value,
                              const std::string& separator = ":");

    /**
     * Append the env with separator (optionally) and value
     * If value or the env value is empty string, or the env has not been defin-
     * ed yet, separator will not be appended.
     */
    virtual Status AppendEnv(const std::string& key, const std::string& value,
                             const std::string& separator = ":");

    /**
    * Set limit on the consumption of variety of resource.
    * @param key the resource key string, the following resource are defined:
    * core   - limits the core file size (KB)
    * data   - max data size (KB)
    * fsize  - maximum filesize (KB)
    * nofile - max number of open files
    * stack  - max stack size (KB)
    * cpu    - max CPU time (MIN)
    * as     - address space limit
    * @param value the resource soft limit, -1 means no limit.
    */
    virtual Status SetLimit(const std::string& key, const std::string& value);

    /**
     * Get the limit value on the consumption of variety of resource
     */
    virtual Status GetLimit(const std::string& key, std::string* value);

    /**
     * Get real path of current program
     */
    virtual std::string GetSelfPath();

    /**
     * Get parent directory of current program
     */
    virtual std::string GetSelfDir();

    /**
     * Get name of current program
     */
    virtual std::string GetSelfName();

    static void Reset(Env* env);

    static Env* Instance();

protected:
    Env() {}
    virtual ~Env() {}

private:
    int getLimitResourceKey(const std::string& key);

private:
    static Env* sInstance;
    DISALLOW_COPY_AND_ASSIGN(Env);
};

#endif  // _SRC_BASE_ENV_H
