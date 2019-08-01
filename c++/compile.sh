workingDir=`pwd`

includeDir="$workingDir/thirdparty/include"
srcFiles="src/base/gettime.cpp                          \
          src/base/cpu.cpp                              \
          src/common/logging.cpp                        \
          src/memory/memcache.cpp"
testFiles="src/base/test/intrusive_list_test.cpp        \
           src/base/test/intrusive_heap_test.cpp        \
           src/memory/test/memcache_test.cpp            \
           test/unittest/main.cpp"
allFiles="$srcFiles $testFiles"

libDir="$workingDir/thirdparty/lib"
libs="$libDir/libgtest.so"

outputDir="build"
target="$outputDir/unittest"
parameters="-Wall -g -std=c++11 -I$workingDir -I$includeDir -L$libDir $libs"

g++ -o $target $allFiles $parameters

# cp $libs $outputDir
