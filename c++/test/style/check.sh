#!/usr/bin

set -e

CPPLINT_DIR=$(dirname $(readlink -f ${BASH_SOURCE[0]}))
CPPLINT_EXE="$CPPLINT_DIR/cpplint.py"
CPPLINT_RULE="$CPPLINT_DIR/cpplint_rule"
PROGRAM_DIR=$(cd $CPPLINT_DIR/../../../; pwd)

echo $PROGRAM_DIR/$dir

cpus=`cat /proc/cpuinfo |grep processor |wc -l`

for dir in `cat $CPPLINT_DIR/cpplint_list`; do
    echo "Check directory: $dir"
    find $PROGRAM_DIR/$dir -name '*.h' -or -name '*.cpp' | xargs -r -n1 -P$cpus $CPPLINT_EXE `cat $CPPLINT_RULE`
    echo "PASS"
done
