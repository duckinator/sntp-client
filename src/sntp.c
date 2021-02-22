#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "socket.h"

// The delta is negative, so be sure to invert any operations on it.
static uint32_t NTP_TIMESTAMP_DELTA = 2208988800;
static uint8_t SNTP_CLIENT_MODE = 3;
static uint8_t SNTP_VERSION = 4 << 3;

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

uint64_t _htonll(uint64_t n) {
    uint32_t *bytes = (uint32_t*)&n;
    bytes[0] = htonl(bytes[0]);
    bytes[1] = htonl(bytes[1]);
    return *(uint64_t*)bytes;
}

uint64_t _ntohll(uint64_t n) {
    uint32_t *bytes = (uint32_t*)&n;
    bytes[0] = ntohl(bytes[0]);
    bytes[1] = ntohl(bytes[1]);
    return *(uint64_t*)bytes;
}

void verify_or_exit(NtpPacket *request, NtpPacket *response) {
    static uint8_t LI_MASK = 3; // 0b00000011
    //static uint8_t VN_MASK = 28; // 0b00011100
    static uint8_t MODE_MASK = 7; // 0b00000111
    static uint8_t VERSION_MASK = 56; // 0b00111000

    if (request->tx_timestamp != response->origin_timestamp) {
        fprintf(stderr, "sntp: Server gave invalid origin timestamp\n");
        exit(EXIT_FAILURE);
    }

    uint8_t mode = (response->li_vn_mode & MODE_MASK);
    uint8_t li = (response->li_vn_mode & LI_MASK) >> 6;
    uint8_t response_version = (response->li_vn_mode & VERSION_MASK) >> 3;
    uint8_t request_version = (request->li_vn_mode & VERSION_MASK) >> 3;

    // LI    Meaning
    // ----------------------------------------------
    // 0     No warning
    // 1     Last minute has 61 seconds
    // 2     Last minute has 59 seconds
    // 3     Alarm condition (clock not synchronized)
    if (li == 3) {
        fprintf(stderr, "sntp: Server clock not synchronized\n");
        exit(EXIT_FAILURE);
    }
    if (li > 3) {
        fprintf(stderr, "sntp: Server gave invalid Leap Indicator value (expected 0-3, got %u)\n", li);
        exit(EXIT_FAILURE);
    }

    // Mode     Meaning
    // ----------------------------
    // 0        Reserved
    // 1        Symmetric active
    // 2        Symmetric passive
    // 3        Client
    // 4        Server
    // 5        Broadcast
    // 6        Reserved for NTP control message
    // 7        Reserved for private use
    if (mode != 4 && mode != 5) {
        fprintf(stderr, "sntp: SNTP server gave invalid MODE (%u)\n", mode);
        exit(EXIT_FAILURE);
    }

    if (response_version != request_version) {
        fprintf(stderr, "sntp: (S)NTP version mismatch: %u (client) != %u (server)\n", request_version, response_version);
        exit(EXIT_FAILURE);
    }

    if (response->stratum == 0) {
        fprintf(stderr, "sntp: STRATUM should not be zero\n");
        exit(EXIT_FAILURE);
    }
}

void calculate_delay(NtpTimestamp *delay, NtpTimestamp *offset,
        NtpPacket *response, uint64_t recv_timestamp) {
    // Time request is sent by client.
    uint64_t t1 = response->origin_timestamp;
    // Time request received by server.
    uint64_t t2 = response->recv_timestamp;
    // Time reply sent by server.
    uint64_t t3 = response->tx_timestamp;
    // Time reply received by client
    uint64_t t4 = recv_timestamp;

    uint64_t delay_raw = (t4 - t1) - (t3 - t2);
    uint64_t offset_raw = ((t2 - t1) + (t4 - t3)) / 2;

    NtpTimestamp *delay_ts = (NtpTimestamp*)&delay_raw;
    NtpTimestamp *offset_ts = (NtpTimestamp*)&offset_raw;

    delay->seconds = delay_ts->seconds;
    delay->seconds_fraction = delay_ts->seconds_fraction;

    offset->seconds = offset_ts->seconds;
    offset->seconds_fraction = offset_ts->seconds_fraction;
}

uint64_t ntp_time() {
    return (uint64_t)time(NULL) + NTP_TIMESTAMP_DELTA;
}

uint32_t ntp_to_unix_time(uint32_t ntp) {
    return ntp - NTP_TIMESTAMP_DELTA;
}

void correct_endianness(NtpPacket *packet) {
    //uint8_t li_vn_mode;
    //uint8_t stratum;
    //uint8_t poll;
    //uint8_t precision;
    packet->root_delay = ntohl(packet->root_delay);
    packet->root_dispersion = ntohl(packet->root_dispersion);
    packet->ref_id = ntohl(packet->ref_id);

    packet->ref_timestamp = _ntohll(packet->ref_timestamp);
    packet->origin_timestamp = _ntohll(packet->origin_timestamp);
    packet->recv_timestamp = _ntohll(packet->recv_timestamp);
    packet->tx_timestamp = _ntohll(packet->tx_timestamp);
}

void sntp_request(NtpResult *result, int debug, const char *server, char *port) {
    NtpPacket request = {
        .li_vn_mode=SNTP_CLIENT_MODE | SNTP_VERSION,
        .tx_timestamp=_htonll(ntp_time()),
    };
    NtpPacket response = {0};

    int sock = socket_create(server, port);

    socket_write(sock, &request, sizeof(NtpPacket));
    socket_read(sock, &response, sizeof(NtpPacket));
    correct_endianness(&request);
    correct_endianness(&response);

    if (debug) {
        fprintf(stderr, "li_vn_mode = %u\n", response.li_vn_mode);
        fprintf(stderr, "stratum    = %u\n", response.stratum);
        fprintf(stderr, "poll       = %u\n", response.poll);
        fprintf(stderr, "root_delay = %u\n", response.root_delay);
        fprintf(stderr, "root_dispersion = %u\n", response.root_dispersion);
        fprintf(stderr, "ref_id     = %u\n", response.ref_id);
        fprintf(stderr, "ref_timestamp    = %lu\n", response.ref_timestamp);
        fprintf(stderr, "origin_timestamp = %lu\n", response.origin_timestamp);
        fprintf(stderr, "recv_timestamp   = %lu\n", response.recv_timestamp);
        fprintf(stderr, "tx_timestamp     = %lu\n", response.tx_timestamp);
    }

    uint64_t receive_time = ntp_time();
    verify_or_exit(&request, &response);

    NtpTimestamp delay, offset;
    calculate_delay(&delay, &offset, &response, receive_time);

    NtpTimestamp *timestamp = (NtpTimestamp*)&response.tx_timestamp;
    timestamp->seconds = ntp_to_unix_time(timestamp->seconds);

    if (debug) {
        fprintf(stderr, "timestamp: %u.%u\n", timestamp->seconds, timestamp->seconds_fraction);
        fprintf(stderr, "delay  = %u.%u\n", delay.seconds, delay.seconds_fraction);
        fprintf(stderr, "offset = %u.%u\n", offset.seconds, offset.seconds_fraction);
    }

    memcpy((void*)&result->timestamp, timestamp, sizeof(NtpTimestamp));
    memcpy((void*)&result->delay, &delay, sizeof(NtpTimestamp));
    memcpy((void*)&result->offset, &offset, sizeof(NtpTimestamp));
}

int main() {
    NtpResult result = {0};
    sntp_request(&result, 0, "pool.ntp.org", "123");
    printf("%u.%u\n", result.timestamp.seconds, result.timestamp.seconds_fraction);
}
