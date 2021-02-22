#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "socket.h"

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

typedef struct NtpTimestamp_s {
    uint32_t seconds;
    uint32_t seconds_fraction;
} NtpTimestamp;

static uint32_t NTP_TIMESTAMP_DELTA = 2208988800;
static uint8_t SNTP_CLIENT_MODE = 3;
static uint8_t SNTP_VERSION = 4 << 3;
static uint8_t LI_MASK = 3; // 0b00000011
static uint8_t VN_MASK = 28; // 0b00011100
static uint8_t MODE_MASK = 224; // 0b11100000

int main() {
    char *server = "pool.ntp.org";
    int port = 123;

    NtpPacket request = {.li_vn_mode=SNTP_CLIENT_MODE | SNTP_VERSION};
    NtpPacket response;

    int sock = socket_create(server, port);

    socket_write(sock, &request, sizeof(NtpPacket));
    socket_read(sock, &response, sizeof(NtpPacket));

    printf("li_vn_mode = %u\n", response.li_vn_mode);
    printf("stratum    = %u\n", response.stratum);
    printf("poll       = %u\n", response.poll);
    printf("root_delay = %u\n", response.root_delay);
    printf("root_dispersion = %u\n", response.root_dispersion);
    printf("ref_id     = %u\n", response.ref_id);
    printf("ref_timestamp    = %lu\n", response.ref_timestamp);
    printf("origin_timestamp = %lu\n", response.origin_timestamp);
    printf("recv_timestamp   = %lu\n", response.recv_timestamp);
    printf("tx_timestamp     = %lu\n", response.tx_timestamp);

    return 0;
}
