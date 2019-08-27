MODE=$1

workingDir=`pwd`
includeDir="$workingDir/thirdparty/include"
srcFiles="src/base/gettime.cpp                          \
          src/base/crc32c.cpp                           \
          src/base/cpu.cpp                              \
          src/base/bit_map.cpp                          \
          src/common/errorcode.cpp                      \
          src/memory/memcache.cpp                       \
          src/string/string_util.cpp                    \
          src/string/dmg_fp/dtoa.cpp                    \
          src/string/dmg_fp/g_fmt.cpp"
testFiles="src/base/test/intrusive_list_test.cpp        \
           src/base/test/intrusive_heap_test.cpp        \
           src/base/test/intrusive_rbtree_test.cpp      \
           src/base/test/intrusive_map_test.cpp         \
           src/base/test/intrusive_hash_map_test.cpp    \
           src/base/test/skiplist_test.cpp              \
           src/base/test/crc32c_test.cpp                \
           src/base/test/bit_map_test.cpp               \
           src/common/test/errorcode_test.cpp           \
           src/memory/test/arena_test.cpp               \
           src/memory/test/memcache_test.cpp            \
           src/memory/test/objcache_test.cpp            \
           src/string/test/string_util_test.cpp         \
           test/unittest/main.cpp"
allFiles="$srcFiles $testFiles"

libDir="$workingDir/thirdparty/lib"
libs="$libDir/libgtest.so $libDir/libtcmalloc.a -lpthread"

outputDir="build/"
target="$outputDir/unittest"
instructionSet="-mcrc32 -msse2 -msse4.1 -msse4.2 -mpclmul"
warns="-Wall -Wno-unused-function"
parameters="-std=c++11 -I$workingDir -I$includeDir $libs $instructionSet $warns"
if [ $MODE ] && [ $MODE == "release" ]; then
    parameters=$parameters" -O2 "
else
    parameters=$parameters" -D__DEBUG__ -g"
fi

g++ -o $target $allFiles $parameters

# for lib in $libs; do
    # cp $lib $outputDir
# done
