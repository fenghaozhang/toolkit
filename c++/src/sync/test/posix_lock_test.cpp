#include <gtest/gtest.h>

#include "src/sync/test/lock_common_test.h"

class PosixLockTestFixture : public LockTestFixture
{
public:
    PosixLockTestFixture() {}
};

TEST_F(PosixLockTestFixture, PosixLock)
{
    // Normal lock
    testLock<NullLock>();
    testLock<SpinLock>();
    testLock<SimpleMutex>();
    testLock<RestrictMutex>();
    // testLock<RecursiveMutex>();
    testLock<AdaptiveMutex>();

    // Read-Write lock
    testRWLock<RWLock>();
}

TEST_F(PosixLockTestFixture, Timeout)
{
    testRWLockTimeout<RWLock>();
}
