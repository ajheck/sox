#include "minunit.h"
#include "sox.h"

// Access private functions
extern void *sox_memcpy(void *dest, const void *src, size_t size);
extern uint16_t sox_strlen(const char *str);
extern int sox_checksum(uint16_t initial_value, const char *data, size_t len);
extern char sox_ntoc(uint8_t nibble);
extern uint8_t sox_cton(char c);

MU_TEST(test_sox_strlen)
{
    char *str = "The length of this string should be 38";
    mu_assert_int_eq(38, sox_strlen(str));
}

MU_TEST(test_sox_memcpy)
{
    char *source = "This string should show up in output after the memcpy";
    uint8_t output[1000];
    memset(output, 0, sizeof(output));
    void *ret = sox_memcpy(output, source, strlen(source));
    mu_assert(ret == output, "sox_memcpy did not return a pointer to dest arg");
    mu_assert_string_eq(source, output);
}

MU_TEST(test_sox_checksum)
{
    int initial_value = 123;
    mu_assert_int_eq(initial_value, sox_checksum(initial_value, NULL, 0));

    char sample_data[] = {31, 12, 41, 12};
    mu_assert_int_eq(219, sox_checksum(initial_value, sample_data, sizeof(sample_data)));
}

MU_TEST(test_sox_ntoc)
{
    uint8_t nibble = 0;
    char c = '0';
    while (nibble < 10)
    {
        mu_assert_int_eq(c++, sox_ntoc(nibble++));
    }
    nibble = 10;
    c = 'A';
    while (nibble < 16)
    {
        mu_assert_int_eq(c++, sox_ntoc(nibble++));
    }    
}

MU_TEST(test_sox_cton)
{
    uint8_t nibble = 0;
    char c = '0';
    while (nibble < 10)
    {
        mu_assert_int_eq(nibble++, sox_cton(c++));
    }
    nibble = 10;
    c = 'A';
    while (nibble < 16)
    {
        mu_assert_int_eq(nibble++, sox_cton(c++));
    }   
    nibble = 10;
    c = 'a';
    while (nibble < 16)
    {
        mu_assert_int_eq(nibble++, sox_cton(c++));
    }     
}

MU_TEST_SUITE(test_sox_helpers) {
	MU_RUN_TEST(test_sox_strlen);
	MU_RUN_TEST(test_sox_memcpy);
	MU_RUN_TEST(test_sox_checksum);
	MU_RUN_TEST(test_sox_ntoc);
	MU_RUN_TEST(test_sox_cton);
}

int test_helpers(void)
{
	MU_RUN_SUITE(test_sox_helpers);
	MU_REPORT();
	return MU_EXIT_CODE;
}