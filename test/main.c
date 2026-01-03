#include <stdio.h>

extern int test_helpers(void);
extern int test_inits(void);
extern int test_send(void);

int main(int argc, char **argv)
{
    typedef int (*test_case_function_t)(void);
    test_case_function_t test_cases[] = {
        test_helpers,
        test_inits,
        test_send,
    };

    int test_count = sizeof(test_cases) / sizeof(test_cases[0]);
    for (int i = 0; i < test_count; i++)
    {
        printf("Running sub-test %d of %d\r\n", i + 1, test_count);
        int result = test_cases[i]();
        if (result)
        {
            return result;
        }
    }

    return 0;
}