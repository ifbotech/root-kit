#include "rk_test.h"

int         rk_t_run = 0;
int         rk_t_failed = 0;
int         rk_t_suite_run = 0;
int         rk_t_suite_failed = 0;
const char *rk_t_suite = "(sin suite)";

#define MAX_SUITES 16
static struct { const char *name; int run; int failed; } s_log[MAX_SUITES];
static int s_nsuites = 0;

void rk_t_begin(const char *name)
{
    rk_t_suite        = name;
    rk_t_suite_run    = 0;
    rk_t_suite_failed = 0;
}

void rk_t_end(void)
{
    if (s_nsuites < MAX_SUITES) {
        s_log[s_nsuites].name   = rk_t_suite;
        s_log[s_nsuites].run    = rk_t_suite_run;
        s_log[s_nsuites].failed = rk_t_suite_failed;
        s_nsuites++;
    }
}

void rk_t_pass(void)
{
    rk_t_run++;
    rk_t_suite_run++;
}

void rk_t_fail(const char *label, const char *detail)
{
    rk_t_run++;
    rk_t_suite_run++;
    rk_t_failed++;
    rk_t_suite_failed++;
    printf("  FALLA  [%s] %s\n         %s\n", rk_t_suite, label, detail);
}

int rk_t_report(void)
{
    int i;
    printf("\n");
    printf("  %-28s %8s %8s\n", "SUITE", "PRUEBAS", "FALLAS");
    printf("  ---------------------------- -------- --------\n");
    for (i = 0; i < s_nsuites; i++) {
        printf("  %-28s %8d %8d%s\n", s_log[i].name, s_log[i].run,
               s_log[i].failed, s_log[i].failed ? "  <<<" : "");
    }
    printf("  ---------------------------- -------- --------\n");
    printf("  %-28s %8d %8d\n\n", "TOTAL", rk_t_run, rk_t_failed);
    if (rk_t_failed == 0) {
        printf("  todo en verde\n\n");
    }
    return rk_t_failed == 0 ? 0 : 1;
}
