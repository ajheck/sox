
#ifndef SOX_H
#define SOX_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#include <string.h>

#ifndef SOX_MAX_PAYLOAD_SIZE
/// @brief Maximum size payload can be.
/// Determines SOX_MSG_BUFFER_SIZE.
#define SOX_MAX_PAYLOAD_SIZE 128
#endif
#if SOX_MAX_PAYLOAD_SIZE <= 0
#error "SOX_MAX_PAYLOAD_SIZE must be greater than 0."
#endif

#ifndef SOX_MAX_SUBJECT_SIZE
/// @brief Maximum size the subject will be.
/// Determines SOX_MSG_BUFFER_SIZE.
#define SOX_MAX_SUBJECT_SIZE 16
#endif
#if SOX_MAX_SUBJECT_SIZE < 4
#error "SOX_MAX_SUBJECT_SIZE is not large enough to fit reply and logging subjects."
#endif

#ifndef SOX_ERR_START
/// @brief Error start code; all sox_ret_t error codes will be less than this.
/// Redefine if it conflicts with IO or other error codes defined in the sytem.
#define SOX_ERR_START (-1000)
#endif
#if SOX_ERR_START > 0
#error "SOX_ERR_START must be less than or equal to 0."
#endif

/// @brief Sox return codes. Values less than 0 indicate errors.
typedef enum
{
    SOX_OK = 0,
    SOX_ERR_NULL_PTR = SOX_ERR_START - 1,
    SOX_ERR_LOCK_FAIL = SOX_ERR_START - 2,
    SOX_ERR_UNLOCK_FAIL = SOX_ERR_START - 3,
} sox_ret_t;

/// @brief Sox logging levels
typedef enum
{
    SOX_LOG_DEBUG = 0,
    SOX_LOG_INFO = 1,
    SOX_LOG_ERROR = 2
} sox_log_level_t;

typedef struct sox_s sox_t;

typedef int (*sox_lock_t)(int enable);
typedef int (*sox_read_t)(char *rx_data, size_t rx_len);
typedef int (*sox_write_t)(const char *tx_data, size_t tx_len);

typedef struct
{
    sox_lock_t write_lock;
    sox_write_t write;
    sox_read_t read;
} sox_config_t;

struct sox_s
{
    uint8_t escaping;
    uint8_t sequence;
    size_t rx_buff_len;
    size_t rx_idx;
    char *rx_buff;
    sox_config_t _cfg;
};

typedef struct
{
    uint8_t sequence;
    char type;
    size_t buff_len;
    char *buff;
} sox_msg_t;

int sox_init(sox_t *s, sox_config_t *cfg, char *buff, size_t buff_len);

int sox_log(sox_t *s, sox_log_level_t level, char *str);

int sox_msg_init(sox_msg_t *m, char *subject, char *buff, size_t buff_len);

char *sox_msg_subject(sox_msg_t *m);

#ifdef __cplusplus
}
#endif
#endif