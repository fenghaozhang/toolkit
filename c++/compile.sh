MODE=$1

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
libs="$libDir/libgtest.so $libDir/libtcmalloc.a -lpthread"

outputDir="build/"
target="$outputDir/unittest"
parameters="-Wall -std=c++11 -I$workingDir -I$includeDir $libs"
if [ $MODE == "DEBUG" ]; then
    parameters=$parameters" -DDEBUG "
else
    parameters=$parameters" -O2 "
fi

g++ -o $target $allFiles $parameters

for lib in $libs; do
    cp $lib $outputDir
done
