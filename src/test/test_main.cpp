#include <cstdio>

extern int run_path_validate_tests(void);
extern int run_krp_validate_tests(void);

int main(void)
{
    int failures = 0;
    failures += run_path_validate_tests();
    failures += run_krp_validate_tests();

    if (failures != 0)
    {
        std::fprintf(stderr, "%d test assertion(s) failed\n", failures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
