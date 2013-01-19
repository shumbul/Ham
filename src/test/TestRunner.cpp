/*
 * Copyright 2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "test/TestRunner.h"

#include <stdio.h>

#include "test/RunnableTest.h"
#include "test/TestException.h"
#include "test/TestSuite.h"


namespace ham {
namespace test {


TestRunner::TestRunner()
	:
	fTestsToRun(),
	fEnvironment(NULL),
	fCurrentTest(NULL),
	fCurrentTestFixture(NULL)
{
}


bool
TestRunner::AddTest(Test* test, const std::string& testCasePath)
{
	if (test == NULL)
		return false;

	if (testCasePath.empty()) {
		fTestsToRun.push_back(TestIdentifier(test));
		return true;
	}

	size_t separatorIndex = testCasePath.find("::");
	if (separatorIndex == std::string::npos) {
		RunnableTest* runnableTest = dynamic_cast<RunnableTest*>(test);
		if (runnableTest != NULL) {
			int testCaseIndex = runnableTest->IndexOfTestCase(testCasePath);
			if (testCaseIndex < 0)
				return false;

			fTestsToRun.push_back(TestIdentifier(test, testCaseIndex));
			return true;
		}
	}

	TestSuite* testSuite = dynamic_cast<TestSuite*>(test);
	if (testSuite == NULL)
		return false;

	Test* subTest = testSuite->GetTest(
		std::string(testCasePath, 0, separatorIndex));
	if (subTest == NULL)
		return false;

	return AddTest(subTest, std::string(testCasePath, separatorIndex + 2));
}


void
TestRunner::Run(TestEnvironment* environment)
{
	fEnvironment = environment;
	fPassedTests = 0;
	fFailedTests = 0;
	fSkippedTests = 0;

	for (TestIdentifierList::const_iterator it = fTestsToRun.begin();
		it != fTestsToRun.end(); ++it) {
		_RunTest(it->GetTest(), it->TestCaseIndex());
	}

	_CleanupFixture();

	fEnvironment = NULL;

	size_t totalTests = fPassedTests + fFailedTests;
	printf("--------\n");
	printf("Summary: %zu tests run, %zu passed, %zu failed, %zu skipped\n",
		totalTests, fPassedTests, fFailedTests, fSkippedTests);
}


void
TestRunner::_RunTest(Test* test, int testCase)
{
	if (RunnableTest* runnableTest = dynamic_cast<RunnableTest*>(test)) {
		// run specific test case, if given
		if (testCase >= 0) {
			_RunTestCase(runnableTest, testCase);
			return;
		}

		// run all test cases
		int count = runnableTest->CountTestCases();
		for (int i = 0; i < count; i++)
			_RunTestCase(runnableTest, i);
	}

	// run the whole test suite
	TestSuite* testSuite = dynamic_cast<TestSuite*>(test);
	if (testSuite == NULL)
		return;

	int count = testSuite->CountTests();
	for (int i = 0; i < count; i++)
		_RunTest(testSuite->TestAt(i), -1);
}


void
TestRunner::_RunTestCase(RunnableTest* test, int testCase)
{
	try {
		printf("%s: ", test->TestCaseAt(testCase, true).c_str());
		fflush(stdin);
		if (fEnvironment->JamExecutable().empty() || test->IsJammable()) {
			_InitFixture(test);
			test->RunTestCase(fEnvironment, fCurrentTestFixture, testCase);
			fPassedTests++;
			printf("PASSED\n");
		} else {
			fSkippedTests++;
			printf("SKIPPED\n");
		}
	} catch (TestException& exception) {
		fFailedTests++;
		printf("FAILED\n");
		printf("%s:%d:\n  %s\n", exception.File(), exception.Line(),
			exception.Message().c_str());
	}
}


void
TestRunner::_InitFixture(RunnableTest* test)
{
	if (test == fCurrentTest)
		return;

	_CleanupFixture();

	fCurrentTestFixture = test->CreateFixture(fEnvironment);
	fCurrentTest = test;
}


void
TestRunner::_CleanupFixture()
{
	if (fCurrentTest != NULL && fCurrentTestFixture != NULL)
		fCurrentTest->DeleteFixture(fEnvironment, fCurrentTestFixture);
	fCurrentTest = NULL;
	fCurrentTestFixture = NULL;
}


}	// namespace test
}	// namespace ham
