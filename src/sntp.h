#ifndef SNTP_H
#define SNTP_H

#include <stdint.h>

typedef struct NtpTimestamp_s {
    uint32_t seconds;
    uint32_t seconds_fraction;
} NtpTimestamp;

typedef struct NtpPacket_s {
    // LI, VN, and Mode are 3 different things.
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint64_t ref_timestamp;
    uint64_t origin_timestamp;
    uint64_t recv_timestamp;
    uint64_t tx_timestamp;
/*
    uint32_t key_id; // optional
    uint64_t msg_digest_1; // optional
    uint64_t msg_digest_2; // optional
*/
} NtpPacket;

typedef struct NtpResult_s {
    NtpTimestamp timestamp;
    NtpTimestamp delay;
    NtpTimestamp offset;
} NtpResult;

void sntp_request(NtpResult *result, int debug, const char *server, char *port);

#endif
