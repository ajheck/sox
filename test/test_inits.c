#include "sox.h"
#include "minunit.h"
#include <stdio.h>

int nop_lock(int enable)
{
    (void)(enable);
    return SOX_OK;
}

int nop_read(char *rx_data, size_t rx_len)
{
    (void)(rx_data);
    (void)(rx_len);
    return SOX_OK;
}

int nop_write(const char *tx_data, size_t tx_len)
{
    (void)(tx_data);
    (void)(tx_len);
    return SOX_OK;
}

sox_config_t valid_config = {
    .read = nop_read,
    .write = nop_write,
    .write_lock = nop_lock,
};

sox_t s;

MU_TEST(test_sox_init_null_args)
{
    char buff[10];
    mu_assert_int_eq(SOX_ERR_NULL_PTR, sox_init(NULL, &valid_config, buff, sizeof(buff)));
    mu_assert_int_eq(SOX_ERR_NULL_PTR, sox_init(&s, NULL, buff, sizeof(buff)));
}

MU_TEST(test_sox_init_good_args)
{
    char buff[10];
    mu_assert_int_eq(SOX_OK, sox_init(&s, &valid_config, buff, sizeof(buff)));

    sox_config_t no_lock = valid_config;
    no_lock.write_lock = NULL;
    mu_assert_int_eq(SOX_OK, sox_init(&s, &no_lock, buff, sizeof(buff)));
}

MU_TEST_SUITE(suite_sox_init)
{
    MU_RUN_TEST(test_sox_init_null_args);
    MU_RUN_TEST(test_sox_init_good_args);
}

int test_inits(void)
{   
	MU_RUN_SUITE(suite_sox_init);
	MU_REPORT();
	return MU_EXIT_CODE;
}