#include "unity.h"
#include "unity_fixture.h"

static void RunAllTests(void) {
  RUN_TEST_GROUP(RC)
  RUN_TEST_GROUP(MOS6502)
}

int main(int argc, const char* argv[]) { return UnityMain(argc, argv, RunAllTests); }
