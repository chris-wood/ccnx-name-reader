#include <stdio.h>

#include <LongBow/unit-test.h>
#include <LongBow/debugging.h>

#include <parc/algol/parc_SafeMemory.h>
#include <parc/algol/parc_Memory.h>

#include "../ccnx_NameReader.c"

LONGBOW_TEST_RUNNER(test_ccnx_NameReader)
{
    // The following Test Fixtures will run their corresponding Test Cases.
    // Test Fixtures are run in the order specified, but all tests should be idempotent.
    // Never rely on the execution order of tests or share state between them.
    LONGBOW_RUN_TEST_FIXTURE(Global);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(test_ccnx_NameReader)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(test_ccnx_NameReader)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, ccnxNameReader_Create);
    LONGBOW_RUN_TEST_CASE(Global, ccnxNameReader_Read);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDOUT_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Global, ccnxNameReader_Create)
{
    CCNxNameReader *reader = ccnxNameReader_Create("test_uris.txt");
    assertNotNull(reader, "Expected a non-NULL reader");
    ccnxNameReader_Release(&reader);
}

LONGBOW_TEST_CASE(Global, ccnxNameReader_Read)
{
    CCNxNameReader *reader = ccnxNameReader_Create("test_uris.txt");
    assertNotNull(reader, "Expected a non-NULL reader");

    PARCIterator *iterator = ccnxNameReader_Iterator(reader);
    while (parcIterator_HasNext(iterator)) {
        CCNxName *name = (CCNxName *) parcIterator_Next(iterator);
        char *nameString = ccnxName_ToString(name);
        printf("Read: %s\n", nameString);
        parcMemory_Deallocate((void **) &nameString);
        ccnxName_Release(&name);
    }
    parcIterator_Release(&iterator);

    ccnxNameReader_Release(&reader);
}

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(test_ccnx_NameReader);
    int exitStatus = LONGBOW_TEST_MAIN(argc, argv, testRunner);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
