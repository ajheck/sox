#include "sox.h"
#include <stddef.h>

#define CHECKSUM_SIZE (4)
#define TYPE_SEQ_SIZE (3)
#define LOG_LEVEL_SIZE (3)
#define SOX_CHECKSUM_INIT (-1)
#define ESC ('\r')
#define END ('\n')
#define DELIM (' ')
static const char *delim_str = " ";
static const char *esc_str = "\r\r";
static const char *msg_end_str = "\r\n";
static const char *log_info_subject = "INFO";
static const char *log_level_lut[] = {
    [SOX_LOG_DEBUG] = "DBG",
    [SOX_LOG_INFO] = "INF",
    [SOX_LOG_ERROR] = "ERR", };

#define RET_ON_NEG(statement) \
    do                        \
    {                         \
        int rc = (statement); \
        if (rc < 0)           \
            return rc;        \
    } while (0)

#ifdef SOX_MAKE_HELPERS_EXTERN
#define HELPER_VISIBILITY
#else
#define HELPER_VISIBILITY static inline
#endif

HELPER_VISIBILITY void *sox_memcpy(void *dest, const void *src, size_t size)
{
    return memcpy(dest, src, size);
}

HELPER_VISIBILITY size_t sox_strlen(const char *str)
{
    return strlen(str);
}

HELPER_VISIBILITY uint8_t sox_to_upper(char c)
{
    // Convert any lower case char to its upper
    // case equivalent by clearing the 5th bit.
    // Leaves numerical chars unchanged.
    return c & ~(1 << 5);
}

HELPER_VISIBILITY char sox_ntoc(uint8_t nibble)
{
    static const char lut[16] = {
        '0', '1', '2', '3',
        '4', '5', '6', '7',
        '8', '9', 'A', 'B',
        'C', 'D', 'E', 'F'};
    return lut[nibble & 0x0F];
}

HELPER_VISIBILITY uint8_t sox_cton(char c)
{
    // Convert any lower case to upper case
    c = sox_to_upper(c);

    // Handle alpha or numeric
    c = (c >= 'A') ? c - 'A' + 10 : c - '0';

    // Band to the expected 0-15 range
    return c & 0x0F;
}

HELPER_VISIBILITY uint16_t sox_checksum(uint16_t initial_value, const char *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        initial_value += data[i];
    }
    return initial_value;
}

HELPER_VISIBILITY int lock(sox_lock_t lock)
{
    return lock == NULL ? SOX_OK : lock(1);
}

HELPER_VISIBILITY int unlock(sox_lock_t lock)
{
    return lock == NULL ? SOX_OK : lock(0);
}

HELPER_VISIBILITY int config_valid(sox_config_t *cfg)
{
    return (cfg == NULL || cfg->read == NULL || cfg->write == NULL)
               ? SOX_ERR_NULL_PTR
               : SOX_OK;
}

HELPER_VISIBILITY void increment_sequence(sox_t *s)
{
    s->sequence++;
    if (s->sequence == 1)
    {
        s->sequence++;
    }
}

int sox_send_data(sox_write_t write, const char *data, size_t len)
{
    const char *p = data;
    const char *end = data + len;

    while (p < end)
    {
        const char *start = p;

        // Find run of non-delimiter bytes
        while (p < end && *p != ESC)
        {
            p++;
        }

        // Write contiguous chunk
        if (p > start)
        {
            int rc = write(start, (int)(p - start));
            if (rc != SOX_OK)
                return rc;
        }

        // Encode escape char byte
        if (p < end && *p == ESC)
        {
            int rc = write(esc_str, 2);
            if (rc != SOX_OK)
                return rc;
            p++; // consume escaped character
        }
    }

    return SOX_OK;
}

int sox_send_log_frame(sox_write_t write, sox_log_level_t level, const char *msg)
{
    static const char log_header[] = {
        '0', '0', '0', '0', DELIM, 'L', 'O', 'G', DELIM};

    char frame_header[13];
    sox_memcpy(&frame_header[0], log_header, sizeof(log_header));
    sox_memcpy(&frame_header[sizeof(log_header)], log_level_lut[level], LOG_LEVEL_SIZE);
    frame_header[12] = DELIM;

    // Compute crc and copy it to the start of the message
    uint16_t crc = sox_checksum(SOX_CHECKSUM_INIT, &frame_header[CHECKSUM_SIZE], sizeof(frame_header) - CHECKSUM_SIZE);
    crc = sox_checksum(crc, msg, sox_strlen(msg));
    frame_header[0] = sox_ntoc(crc >> 12);
    frame_header[1] = sox_ntoc(crc >> 8);
    frame_header[2] = sox_ntoc(crc >> 4);
    frame_header[3] = sox_ntoc(crc >> 0);

    RET_ON_NEG(sox_send_data(write, frame_header, sizeof(frame_header)));
    RET_ON_NEG(sox_send_data(write, msg, sox_strlen(msg)));
    RET_ON_NEG(write(msg_end_str, 2));

    return SOX_OK;
}

void sox_rx_reset(sox_t *s)
{
    s->escaping = 0;
    s->rx_idx = 0;
}

int sox_msg_from_raw(sox_msg_t *m, uint8_t *data, size_t len)
{
    // TODO: Add support for "short" headers (messages lacking checksum)

    // Extract checksum
    uint16_t rx_checksum = 0;
    rx_checksum |= sox_cton(data[0]) << 12;
    rx_checksum |= sox_cton(data[1]) << 8;
    rx_checksum |= sox_cton(data[2]) << 4;
    rx_checksum |= sox_cton(data[3]);

    // Compute checksum over recieved data
    uint16_t computed_checksum = sox_checksum(SOX_CHECKSUM_INIT, &data[4], len);

    if(rx_checksum != computed_checksum)
    {
        return -1;
    }

    m->type = data[5];
    m->sequence = 0;
    m->sequence |= sox_cton(data[6]);
    m->sequence |= sox_cton(data[7]);

    return 0;
}

int sox_process_data(sox_t *s, uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (s->rx_idx >= s->rx_buff_len)
        {
            // No more room
            sox_rx_reset(s);
        }

        uint8_t b = data[i];
        if (!s->escaping)
        {
            if (b == ESC)
            {
                // Beginning of escaped sequence
                s->escaping = 1;
            }
            else
            {
                // Normal data byte.
                s->rx_buff[s->rx_idx++] = b;
            }
        }
        else
        {
            // Handle the escaped byte.
            s->escaping = 0;
            if(b == ESC)
            {
                s->rx_buff[s->rx_idx++] = ESC;
            }
            else if(b == END)
            {
                // Valid end of message sequence.
                // Try to make message out of rx buffer.
                sox_msg_t msg;
                int rc = sox_msg_from_raw(&msg, s->rx_buff, s->rx_idx);
                sox_rx_reset(s);
            }
            else
            {
                sox_rx_reset(s);
            }
        }
    }
    return SOX_OK;
}

int sox_init(sox_t *s, sox_config_t *cfg, char *buff, size_t buff_len)
{
    if (config_valid(cfg) != SOX_OK || s == NULL)
    {
        return SOX_ERR_NULL_PTR;
    }
    s->escaping = 0;
    s->_cfg = *cfg;
    s->sequence = 1;
    s->rx_buff = buff;
    s->rx_buff_len = buff_len;
    return SOX_OK;
}

int sox_log(sox_t *s, sox_log_level_t level, char *str)
{
    if (s == NULL || str == NULL)
    {
        return SOX_ERR_NULL_PTR;
    }

    if (lock(s->_cfg.write_lock) != SOX_OK)
    {
        return SOX_ERR_LOCK_FAIL;
    }

    int reuslt = sox_send_log_frame(s->_cfg.write, level, str);

    if (unlock(s->_cfg.write_lock) != SOX_OK)
    {
        return SOX_ERR_LOCK_FAIL;
    }

    return SOX_OK;
}
