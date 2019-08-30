#include <gtest/gtest.h>

#include "src/sync/test/common_test.h"

class MicroLockTestFixture : public LockTestFixture
{
public:
    MicroLockTestFixture() {}
};

TEST_F(MicroLockTestFixture, MicroLock)
{
    // Normal lock
    testLock<MicroLock>();

    // Read-Write lock
    testRWLock<MicroRWLock>();
    testRWLock<MicroRWLockPreferWrite>();
}

TEST_F(MicroLockTestFixture, Timeout)
{
    testLockTimeout<MicroLock>();
    testRWLockTimeout<MicroRWLock>();
    testRWLockTimeout<MicroRWLockPreferWrite>();
}

TEST_F(MicroLockTestFixture, Owner)
{
    testLockOwner<MicroLock>();
    testRWLockOwner<MicroRWLock>();
    testRWLockOwner<MicroRWLockPreferWrite>();
}
