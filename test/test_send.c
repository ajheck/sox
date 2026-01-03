#include "minunit.h"
#include "sox.h"

// Private functions from sox.c
extern int sox_send_data(sox_write_t write, const char *data, size_t len);
extern int sox_checksum(uint16_t initial_value, const char *data, size_t len);
extern int sox_send_log_frame(sox_write_t write, sox_log_level_t level, const char *msg);
extern char sox_ntoc(uint8_t nibble);

char buffer[1000];
int write_index = 0;
int buffer_write(const char *tx_data, size_t tx_len)
{
    int i = 0;
    while(i < tx_len && write_index < sizeof(buffer))
    {
        buffer[write_index++] = tx_data[i++];
    }
    
    return (write_index < sizeof(buffer)) ? SOX_OK : -1;
}

void reset_buffer(void)
{
    memset(buffer, 0, sizeof(buffer));
    write_index = 0;
}

int dummy_read(char *rx_data, size_t rx_len)
{
    (void)rx_data;
    (void)rx_len;
    return SOX_OK;
}

static sox_config_t cfg = {
    .write = buffer_write,
    .read = dummy_read,
    .write_lock = NULL,
};

sox_t g_sox;
char rx_buffer[128];

void reset_sox(void)
{
    reset_buffer();
    sox_init(&g_sox, &cfg, rx_buffer, sizeof(rx_buffer));
}

MU_TEST(test_sox_send_data_no_escapes)
{
    char *test = "test";
    mu_assert_int_eq(SOX_OK, sox_send_data(buffer_write, test, strlen(test)));
    mu_assert_string_eq(test, buffer);
}

MU_TEST(test_sox_send_data_some_escapes)
{
    char *test = "te\rst\n";
    char *expected = "te\r\rst\n";
    mu_assert_int_eq(SOX_OK, sox_send_data(buffer_write, test, strlen(test)));
    mu_assert_string_eq(expected, buffer);
}

MU_TEST(test_sox_send_data_only_escapes)
{
    char *test = "\r\r";
    char *expected = "\r\r\r\r";
    mu_assert_int_eq(SOX_OK, sox_send_data(buffer_write, test, strlen(test)));
    mu_assert_string_eq(expected, buffer);
}

MU_TEST_SUITE(suite_sox_send_data)
{
    MU_SUITE_CONFIGURE(reset_buffer, NULL);
    MU_RUN_TEST(test_sox_send_data_no_escapes);
    MU_RUN_TEST(test_sox_send_data_some_escapes);
    MU_RUN_TEST(test_sox_send_data_only_escapes);
}

void print_unescaped_log_frame(char *msg, char *log_level_str, char *buffer, size_t buffer_size)
{
    char payload[128];
    snprintf(payload, sizeof(payload), " LOG %s %s", log_level_str, msg);
    int expected_crc = sox_checksum(-1, payload, strlen(payload));
    snprintf(buffer, buffer_size, "%04X LOG %s %s\r\n", expected_crc, log_level_str, msg);
}

MU_TEST(test_sox_send_log_frame_no_escapes)
{
    char *message = "This is a log message";
    char expected[128];
    print_unescaped_log_frame(message, "INF", expected, sizeof(expected));
    mu_assert_int_eq(SOX_OK, sox_send_log_frame(buffer_write, SOX_LOG_INFO, message));
    mu_assert_string_eq(expected, buffer);
    
    reset_buffer();
    print_unescaped_log_frame(message, "DBG", expected, sizeof(expected));
    mu_assert_int_eq(SOX_OK, sox_send_log_frame(buffer_write, SOX_LOG_DEBUG, message));
    mu_assert_string_eq(expected, buffer);
    
    reset_buffer();
    print_unescaped_log_frame(message, "ERR", expected, sizeof(expected));
    mu_assert_int_eq(SOX_OK, sox_send_log_frame(buffer_write, SOX_LOG_ERROR, message));
    mu_assert_string_eq(expected, buffer);
}

MU_TEST(test_sox_send_log_frame_some_escapes)
{
    char *message = "This is a log message\r\nWith escapes!";
    char *escaped_message = "This is a log message\r\r\nWith escapes!";
    char payload[128];
    char expected[128];

    snprintf(payload, sizeof(payload), " LOG INF %s", message);
    int expected_crc = sox_checksum(-1, payload, strlen(payload));
    snprintf(expected, sizeof(expected), "%04X LOG INF %s\r\n", expected_crc, escaped_message);
    mu_assert_int_eq(SOX_OK, sox_send_log_frame(buffer_write, SOX_LOG_INFO, message));
    mu_assert_string_eq(expected, buffer);   
}

MU_TEST(test_sox_send_log_frame_all_escapes)
{
    char *message = "\r\r\r\r";
    char *escaped_message = "\r\r\r\r\r\r\r\r";
    char payload[128];
    char expected[128];

    snprintf(payload, sizeof(payload), " LOG INF %s", message);
    int expected_crc = sox_checksum(-1, payload, strlen(payload));
    snprintf(expected, sizeof(expected), "%04X LOG INF %s\r\n", expected_crc, escaped_message);
    mu_assert_int_eq(SOX_OK, sox_send_log_frame(buffer_write, SOX_LOG_INFO, message));
    mu_assert_string_eq(expected, buffer);   
}

MU_TEST_SUITE(suite_sox_send_log_frame)
{
    MU_SUITE_CONFIGURE(reset_buffer, NULL);
    MU_RUN_TEST(test_sox_send_log_frame_no_escapes);
    MU_RUN_TEST(test_sox_send_log_frame_some_escapes);
    MU_RUN_TEST(test_sox_send_log_frame_all_escapes);
}

MU_TEST(test_sox_send_log_str_no_escapes)
{
    char *message = "This is a log message";
    char expected[128];
    print_unescaped_log_frame(message, "INF", expected, sizeof(expected));
    mu_assert_int_eq(SOX_OK, sox_log(&g_sox, SOX_LOG_INFO, message));
    mu_assert_string_eq(expected, buffer);
    
    reset_buffer();
    print_unescaped_log_frame(message, "DBG", expected, sizeof(expected));
    mu_assert_int_eq(SOX_OK, sox_log(&g_sox, SOX_LOG_DEBUG, message));
    mu_assert_string_eq(expected, buffer);
    
    reset_buffer();
    print_unescaped_log_frame(message, "ERR", expected, sizeof(expected));
    mu_assert_int_eq(SOX_OK, sox_log(&g_sox, SOX_LOG_ERROR, message));
    mu_assert_string_eq(expected, buffer);
}

MU_TEST(test_sox_send_log_str_some_escapes)
{
    char *message = "This is a log message\r\nWith escapes!";
    char *escaped_message = "This is a log message\r\r\nWith escapes!";
    char payload[128];
    char expected[128];

    snprintf(payload, sizeof(payload), " LOG INF %s", message);
    int expected_crc = sox_checksum(-1, payload, strlen(payload));
    snprintf(expected, sizeof(expected), "%04X LOG INF %s\r\n", expected_crc, escaped_message);
    mu_assert_int_eq(SOX_OK, sox_log(&g_sox, SOX_LOG_INFO, message));
    mu_assert_string_eq(expected, buffer);   
}

MU_TEST(test_sox_send_log_str_all_escapes)
{
    char *message = "\r\r\r\r";
    char *escaped_message = "\r\r\r\r\r\r\r\r";
    char payload[128];
    char expected[128];

    snprintf(payload, sizeof(payload), " LOG INF %s", message);
    int expected_crc = sox_checksum(-1, payload, strlen(payload));
    snprintf(expected, sizeof(expected), "%04X LOG INF %s\r\n", expected_crc, escaped_message);
    mu_assert_int_eq(SOX_OK, sox_log(&g_sox, SOX_LOG_INFO, message));
    mu_assert_string_eq(expected, buffer);   
}

MU_TEST_SUITE(suite_sox_send_log_str)
{
    MU_SUITE_CONFIGURE(reset_sox, NULL);
    MU_RUN_TEST(test_sox_send_log_str_no_escapes);
    MU_RUN_TEST(test_sox_send_log_str_some_escapes);
    MU_RUN_TEST(test_sox_send_log_str_all_escapes);
}

int test_send(void)
{
	MU_RUN_SUITE(suite_sox_send_data);
    MU_RUN_SUITE(suite_sox_send_log_frame);
    MU_RUN_SUITE(suite_sox_send_log_str);
	MU_REPORT();
	return MU_EXIT_CODE;
}