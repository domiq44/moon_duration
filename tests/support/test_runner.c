/*
 * test_runner.c
 * Runner partagé des suites C : exécution des cas, assertions, rapport
 * PASS/FAIL/SKIP et calcul des totaux.
 */

#include "test_runner.h"

#include <stdarg.h>
#include <stdio.h>

static struct {
  int failed;
  int skipped;
} test_context;

static void runner_context_reset(void) {
  test_context.failed = 0;
  test_context.skipped = 0;
}

void runner_context_fail(const char *file, int line, const char *format, ...) {
  va_list args;

  test_context.failed = 1;

  printf("    [FAIL] %s:%d: ", file, line);
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

void runner_context_skip(const char *reason) {
  test_context.skipped = 1;
  printf("    [SKIP] %s\n", reason != NULL ? reason : "");
}

void runner_context_info(const char *format, ...) {
  va_list args;

  printf("    [INFO] ");
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
}

int runner_run(const char *suite_name, const TestCase *cases, size_t count) {
  int passed = 0;
  int failed = 0;
  int skipped = 0;
  size_t i;

  printf("Running %s...\n", suite_name);

  for (i = 0; i < count; i++) {
    const TestCase *test_case = &cases[i];

    runner_context_reset();
    printf("  [TEST] %s\n", test_case->name);
    test_case->function();

    if (test_context.skipped) {
      printf("  [SKIP] %s\n", test_case->name);
      skipped++;
    } else if (test_context.failed) {
      printf("  [FAIL] %s\n", test_case->name);
      failed++;
    } else {
      printf("  [PASS] %s\n", test_case->name);
      passed++;
    }
  }

  printf("\nResults: %d passed, %d failed, %d skipped (total %d)\n", passed,
         failed, skipped, (int)count);

  if (failed > 0) {
    printf("FAILED!\n");
    return 1;
  }

  printf("ALL TESTS PASSED!\n");
  return 0;
}