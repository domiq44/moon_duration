/*
 * test_runner.h
 * Contrat du runner partagé : déclaration des cas, assertions et bilan
 * PASS/FAIL/SKIP.
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include <stddef.h>
#include <string.h>

typedef struct {
    const char *name;
    void (*function)(void);
} TestCase;

void runner_context_fail(const char *file, int line, const char *format, ...);
void runner_context_skip(const char *reason);
void runner_context_info(const char *format, ...);

int runner_run(const char *suite_name, const TestCase *cases, size_t count);

#define TEST_CASE_COUNT(cases) (sizeof(cases) / sizeof((cases)[0]))

#define TEST_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            runner_context_fail(__FILE__, __LINE__, \
                              "condition fausse : %s", #condition); \
            return; \
        } \
    } while (0)

#define TEST_ASSERT_FALSE(condition) \
    do { \
        if ((condition)) { \
            runner_context_fail(__FILE__, __LINE__, \
                              "condition vraie : %s", #condition); \
            return; \
        } \
    } while (0)

#define TEST_ASSERT_INT_EQ(actual, expected) \
    do { \
        long test_actual_ = (long)(actual); \
        long test_expected_ = (long)(expected); \
        if (test_actual_ != test_expected_) { \
            runner_context_fail(__FILE__, __LINE__, \
                              "attendu %ld, obtenu %ld", \
                              test_expected_, test_actual_); \
            return; \
        } \
    } while (0)

#define TEST_ASSERT_DOUBLE_EQ(actual, expected, tolerance) \
    do { \
        double test_actual_ = (double)(actual); \
        double test_expected_ = (double)(expected); \
        double test_tolerance_ = (double)(tolerance); \
        double test_diff_ = test_actual_ - test_expected_; \
        if (test_diff_ < 0.0) { \
            test_diff_ = -test_diff_; \
        } \
        if (!(test_diff_ <= test_tolerance_)) { \
            runner_context_fail(__FILE__, __LINE__, \
                              "attendu %g +/- %g, obtenu %g", \
                              test_expected_, test_tolerance_, test_actual_); \
            return; \
        } \
    } while (0)

#define TEST_ASSERT_STRING_EQ(actual, expected) \
    do { \
        const char *test_actual_ = (actual); \
        const char *test_expected_ = (expected); \
        if (test_actual_ == NULL || test_expected_ == NULL || \
            strcmp(test_actual_, test_expected_) != 0) { \
            runner_context_fail(__FILE__, __LINE__, \
                              "attendu \"%s\", obtenu \"%s\"", \
                              test_expected_ != NULL ? test_expected_ : "(null)", \
                              test_actual_ != NULL ? test_actual_ : "(null)"); \
            return; \
        } \
    } while (0)

#define TEST_SKIP(reason) \
    do { \
        runner_context_skip(reason); \
        return; \
    } while (0)

#endif /* TEST_RUNNER_H */