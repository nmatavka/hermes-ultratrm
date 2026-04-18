/*
 * UltraTerminal transport adapter for the copied tn5250 runtime.
 */
#ifndef ULTRATERMINAL_TN5250_ULTRA_STREAM_H
#define ULTRATERMINAL_TN5250_ULTRA_STREAM_H

#include "lib5250/stream.h"

#ifdef __cplusplus
extern "C" {
#endif

int tn5250_ultra_stream_init(Tn5250Stream* stream);
int tn5250_ultra_stream_start(Tn5250Stream* stream, unsigned char* out,
        int out_limit, int* out_len);
int tn5250_ultra_stream_feed(Tn5250Stream* stream, const unsigned char* data,
        int data_len, unsigned char* out, int out_limit, int* out_len);
int tn5250_ultra_stream_enable_tls(Tn5250Stream* stream, int verify_tls,
        const char* ca_path, const char* cert_path, const char* key_path,
        char* error, int error_len);
int tn5250_ultra_stream_tls_active(Tn5250Stream* stream);
int tn5250_ultra_stream_begin_output(Tn5250Stream* stream, unsigned char* out,
        int out_limit, int* out_len);
int tn5250_ultra_stream_end_output(Tn5250Stream* stream);

#ifdef __cplusplus
}
#endif

#endif
