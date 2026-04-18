/*
 * UltraTerminal nonblocking TN5250 stream.
 *
 * This file adapts tn5250's Telnet/EOR framing to UltraTerminal's existing
 * socket ownership. Incoming bytes are supplied by comwsock and outgoing bytes
 * are collected into the caller's buffer for ComSndBufrSend().
 */
#include "ultra_stream.h"
#include "lib5250/tn5250-private.h"

#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#define UT5250_SEND    1
#define UT5250_IS      0
#define UT5250_VALUE   1
#define UT5250_VAR     0
#define UT5250_USERVAR 3

#define UT5250_TRANSMIT_BINARY 0
#define UT5250_END_OF_RECORD   25
#define UT5250_TERMINAL_TYPE   24
#define UT5250_TIMING_MARK     6
#define UT5250_NEW_ENVIRON     39

#define UT5250_EOR  239
#define UT5250_SE   240
#define UT5250_SB   250
#define UT5250_WILL 251
#define UT5250_WONT 252
#define UT5250_DO   253
#define UT5250_DONT 254
#define UT5250_IAC  255

#define UT5250_STREAM_STATE_NO_DATA     0
#define UT5250_STREAM_STATE_DATA        1
#define UT5250_STREAM_STATE_HAVE_IAC    2
#define UT5250_STREAM_STATE_HAVE_VERB   3
#define UT5250_STREAM_STATE_HAVE_SB     4
#define UT5250_STREAM_STATE_HAVE_SB_IAC 5

typedef struct ut5250_stream_state
    {
    const unsigned char* in;
    int in_len;
    int in_pos;
    unsigned char* out;
    int out_limit;
    int* out_len;
    unsigned char verb;
    int telnet_started;
#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
    SSL_CTX* tls_ctx;
    SSL* tls_ssl;
    BIO* tls_rbio;
    BIO* tls_wbio;
    int tls_requested;
    int tls_handshake_done;
#endif
    } UT5250_STREAM_STATE;

static int ultra_stream_connect(Tn5250Stream* stream, const char* to);
static void ultra_stream_disconnect(Tn5250Stream* stream);
static int ultra_stream_handle_receive(Tn5250Stream* stream);
static void ultra_stream_send_packet(Tn5250Stream* stream, int length,
        StreamHeader header, unsigned char* data);
static void ultra_stream_destroy(Tn5250Stream* stream);
static int ultra_stream_append(Tn5250Stream* stream, const unsigned char* data,
        int len);
static int ultra_stream_append_raw(Tn5250Stream* stream,
        const unsigned char* data, int len);
static int ultra_stream_append3(Tn5250Stream* stream, unsigned char a,
        unsigned char b, unsigned char c);
static int ultra_stream_start_telnet(Tn5250Stream* stream);
static int ultra_stream_get_byte(Tn5250Stream* stream);
static void ultra_stream_do_verb(Tn5250Stream* stream, unsigned char verb,
        unsigned char what);
static void ultra_stream_sb(Tn5250Stream* stream, unsigned char* sb,
        int sb_len);
static void ultra_stream_sb_var_value(Tn5250Buffer* buf, unsigned char* var,
        unsigned char* value);
static void ultra_stream_escape(Tn5250Buffer* buffer);
static void ultra_stream_error(char* error, int error_len, const char* text);

#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
static int ultra_stream_tls_drive(Tn5250Stream* stream);
static int ultra_stream_tls_drain(Tn5250Stream* stream);
static int ultra_stream_tls_feed(Tn5250Stream* stream, const unsigned char* data,
        int data_len);
static int ultra_stream_tls_write(Tn5250Stream* stream,
        const unsigned char* data, int len);
static int ultra_stream_tls_feed_plain(Tn5250Stream* stream,
        const unsigned char* data, int len);
static int ultra_stream_tls_error(Tn5250Stream* stream, int rc);
#endif

int tn5250_ultra_stream_init(Tn5250Stream* stream)
    {
    UT5250_STREAM_STATE* state;

    state = (UT5250_STREAM_STATE*)malloc(sizeof(UT5250_STREAM_STATE));
    if (state == NULL)
        return -1;

    memset(state, 0, sizeof(*state));
    stream->connect = ultra_stream_connect;
    stream->disconnect = ultra_stream_disconnect;
    stream->handle_receive = ultra_stream_handle_receive;
    stream->send_packet = ultra_stream_send_packet;
    stream->destroy = ultra_stream_destroy;
    stream->userdata = state;
    return 0;
    }

int tn5250_ultra_stream_start(Tn5250Stream* stream, unsigned char* out,
        int out_limit, int* out_len)
    {
    if (tn5250_ultra_stream_begin_output(stream, out, out_limit, out_len) < 0)
        return -1;

#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
    {
    UT5250_STREAM_STATE* state;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state && state->tls_requested)
        {
        if (ultra_stream_tls_drive(stream) < 0)
            return -1;
        return tn5250_ultra_stream_end_output(stream);
        }
    }
#endif

    if (ultra_stream_start_telnet(stream) < 0)
        return -1;

    return tn5250_ultra_stream_end_output(stream);
    }

int tn5250_ultra_stream_feed(Tn5250Stream* stream, const unsigned char* data,
        int data_len, unsigned char* out, int out_limit, int* out_len)
    {
    UT5250_STREAM_STATE* state;
    int rc;

    if (stream == NULL || stream->userdata == NULL)
        return -1;

    if (tn5250_ultra_stream_begin_output(stream, out, out_limit, out_len) < 0)
        return -1;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    state->in = data;
    state->in_len = data_len;
    state->in_pos = 0;

#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
    if (state->tls_requested)
        {
        rc = ultra_stream_tls_feed(stream, data, data_len);
        state->in = NULL;
        state->in_len = 0;
        state->in_pos = 0;
        return rc ? 0 : -1;
        }
#endif

    rc = tn5250_stream_handle_receive(stream);

    state->in = NULL;
    state->in_len = 0;
    state->in_pos = 0;
    return rc ? 0 : -1;
    }

int tn5250_ultra_stream_enable_tls(Tn5250Stream* stream, int verify_tls,
        const char* ca_path, const char* cert_path, const char* key_path,
        char* error, int error_len)
    {
    UT5250_STREAM_STATE* state;

    if (stream == NULL || stream->userdata == NULL)
        {
        ultra_stream_error(error, error_len, "Invalid TN5250 stream.");
        return -1;
        }

    state = (UT5250_STREAM_STATE*)stream->userdata;

#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
    OPENSSL_init_ssl(0, NULL);
    state->tls_ctx = SSL_CTX_new(TLS_client_method());
    if (state->tls_ctx == NULL)
        {
        ultra_stream_error(error, error_len, "Could not create OpenSSL context.");
        return -1;
        }

    if (verify_tls)
        {
        SSL_CTX_set_verify(state->tls_ctx, SSL_VERIFY_PEER, NULL);
        SSL_CTX_set_default_verify_paths(state->tls_ctx);
        if (ca_path && ca_path[0] &&
                SSL_CTX_load_verify_locations(state->tls_ctx, ca_path, NULL) != 1)
            {
            ultra_stream_error(error, error_len, "Could not load TN5250 CA file.");
            return -1;
            }
        }
    else
        {
        SSL_CTX_set_verify(state->tls_ctx, SSL_VERIFY_NONE, NULL);
        }

    if (cert_path && cert_path[0] &&
            SSL_CTX_use_certificate_file(state->tls_ctx, cert_path,
                SSL_FILETYPE_PEM) != 1)
        {
        ultra_stream_error(error, error_len,
                "Could not load TN5250 client certificate.");
        return -1;
        }
    if (key_path && key_path[0] &&
            SSL_CTX_use_PrivateKey_file(state->tls_ctx, key_path,
                SSL_FILETYPE_PEM) != 1)
        {
        ultra_stream_error(error, error_len,
                "Could not load TN5250 client private key.");
        return -1;
        }

    state->tls_ssl = SSL_new(state->tls_ctx);
    state->tls_rbio = BIO_new(BIO_s_mem());
    state->tls_wbio = BIO_new(BIO_s_mem());
    if (state->tls_ssl == NULL || state->tls_rbio == NULL ||
            state->tls_wbio == NULL)
        {
        ultra_stream_error(error, error_len, "Could not initialize TN5250 TLS.");
        return -1;
        }

    SSL_set_bio(state->tls_ssl, state->tls_rbio, state->tls_wbio);
    SSL_set_connect_state(state->tls_ssl);
    state->tls_requested = 1;
    state->tls_handshake_done = 0;
    ultra_stream_error(error, error_len, "");
    return 0;
#else
    (void)state;
    (void)verify_tls;
    (void)ca_path;
    (void)cert_path;
    (void)key_path;
    ultra_stream_error(error, error_len,
            "TN5250 TLS was not compiled into this build.");
    return -1;
#endif
    }

int tn5250_ultra_stream_tls_active(Tn5250Stream* stream)
    {
#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
    UT5250_STREAM_STATE* state;

    if (stream == NULL || stream->userdata == NULL)
        return 0;
    state = (UT5250_STREAM_STATE*)stream->userdata;
    return state->tls_requested && state->tls_handshake_done;
#else
    (void)stream;
    return 0;
#endif
    }

int tn5250_ultra_stream_begin_output(Tn5250Stream* stream, unsigned char* out,
        int out_limit, int* out_len)
    {
    UT5250_STREAM_STATE* state;

    if (stream == NULL || stream->userdata == NULL)
        return -1;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    state->out = out;
    state->out_limit = out_limit;
    state->out_len = out_len;
    if (out_len)
        *out_len = 0;
    return 0;
    }

int tn5250_ultra_stream_end_output(Tn5250Stream* stream)
    {
    UT5250_STREAM_STATE* state;

    if (stream == NULL || stream->userdata == NULL)
        return -1;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    state->out = NULL;
    state->out_limit = 0;
    state->out_len = NULL;
    return 0;
    }

static int ultra_stream_connect(Tn5250Stream* stream, const char* to)
    {
    (void)to;
    stream->state = UT5250_STREAM_STATE_DATA;
    return 0;
    }

static void ultra_stream_disconnect(Tn5250Stream* stream)
    {
    stream->state = UT5250_STREAM_STATE_NO_DATA;
    }

static void ultra_stream_destroy(Tn5250Stream* stream)
    {
    if (stream->userdata)
        {
#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
        UT5250_STREAM_STATE* state;

        state = (UT5250_STREAM_STATE*)stream->userdata;
        if (state->tls_ssl)
            SSL_free(state->tls_ssl);
        if (state->tls_ctx)
            SSL_CTX_free(state->tls_ctx);
#endif
        free(stream->userdata);
        stream->userdata = NULL;
        }
    }

static int ultra_stream_handle_receive(Tn5250Stream* stream)
    {
    int c;

    while ((c = ultra_stream_get_byte(stream)) != -1 && c != -2)
        {
        if (c == -UT5250_END_OF_RECORD && stream->current_record != NULL)
            {
            stream->records =
                tn5250_record_list_add(stream->records, stream->current_record);
            stream->current_record = NULL;
            stream->record_count++;
            continue;
            }

        if (stream->current_record == NULL)
            stream->current_record = tn5250_record_new();

        if (stream->current_record)
            tn5250_record_append_byte(stream->current_record,
                    (unsigned char)c);
        }

    return (c != -2);
    }

static int ultra_stream_next_raw(Tn5250Stream* stream)
    {
    UT5250_STREAM_STATE* state;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state == NULL || state->in == NULL || state->in_pos >= state->in_len)
        return -1;

    return state->in[state->in_pos++];
    }

static int ultra_stream_get_byte(Tn5250Stream* stream)
    {
    int temp;
    UT5250_STREAM_STATE* state;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state == NULL)
        return -2;

    do {
        if (stream->state == UT5250_STREAM_STATE_NO_DATA)
            stream->state = UT5250_STREAM_STATE_DATA;

        temp = ultra_stream_next_raw(stream);
        if (temp < 0)
            return temp;

        switch (stream->state)
            {
        case UT5250_STREAM_STATE_DATA:
            if (temp == UT5250_IAC)
                stream->state = UT5250_STREAM_STATE_HAVE_IAC;
            break;

        case UT5250_STREAM_STATE_HAVE_IAC:
            switch (temp)
                {
            case UT5250_IAC:
                stream->state = UT5250_STREAM_STATE_DATA;
                break;
            case UT5250_DO:
            case UT5250_DONT:
            case UT5250_WILL:
            case UT5250_WONT:
                state->verb = (unsigned char)temp;
                stream->state = UT5250_STREAM_STATE_HAVE_VERB;
                break;
            case UT5250_SB:
                stream->state = UT5250_STREAM_STATE_HAVE_SB;
                tn5250_buffer_free(&(stream->sb_buf));
                tn5250_buffer_init(&(stream->sb_buf));
                break;
            case UT5250_EOR:
                stream->state = UT5250_STREAM_STATE_DATA;
                return -UT5250_END_OF_RECORD;
            default:
                stream->state = UT5250_STREAM_STATE_NO_DATA;
                break;
                }
            break;

        case UT5250_STREAM_STATE_HAVE_VERB:
            ultra_stream_do_verb(stream, state->verb, (unsigned char)temp);
            stream->state = UT5250_STREAM_STATE_NO_DATA;
            break;

        case UT5250_STREAM_STATE_HAVE_SB:
            if (temp == UT5250_IAC)
                stream->state = UT5250_STREAM_STATE_HAVE_SB_IAC;
            else
                tn5250_buffer_append_byte(&(stream->sb_buf),
                        (unsigned char)temp);
            break;

        case UT5250_STREAM_STATE_HAVE_SB_IAC:
            if (temp == UT5250_SE)
                {
                ultra_stream_sb(stream, tn5250_buffer_data(&(stream->sb_buf)),
                        tn5250_buffer_length(&(stream->sb_buf)));
                tn5250_buffer_free(&(stream->sb_buf));
                tn5250_buffer_init(&(stream->sb_buf));
                stream->state = UT5250_STREAM_STATE_NO_DATA;
                }
            else if (temp == UT5250_IAC)
                {
                tn5250_buffer_append_byte(&(stream->sb_buf), UT5250_IAC);
                stream->state = UT5250_STREAM_STATE_HAVE_SB;
                }
            else
                {
                stream->state = UT5250_STREAM_STATE_HAVE_SB;
                }
            break;

        default:
            stream->state = UT5250_STREAM_STATE_DATA;
            break;
            }
    } while (stream->state != UT5250_STREAM_STATE_DATA);

    return temp;
    }

static void ultra_stream_do_verb(Tn5250Stream* stream, unsigned char verb,
        unsigned char what)
    {
    unsigned char reply;

    reply = 0;
    switch (verb)
        {
    case UT5250_DO:
        switch (what)
            {
        case UT5250_TERMINAL_TYPE:
        case UT5250_END_OF_RECORD:
        case UT5250_TRANSMIT_BINARY:
        case UT5250_NEW_ENVIRON:
            reply = UT5250_WILL;
            break;
        default:
            reply = UT5250_WONT;
            break;
            }
        break;

    case UT5250_WILL:
        switch (what)
            {
        case UT5250_TERMINAL_TYPE:
        case UT5250_END_OF_RECORD:
        case UT5250_TRANSMIT_BINARY:
        case UT5250_NEW_ENVIRON:
            reply = UT5250_DO;
            break;
        case UT5250_TIMING_MARK:
        default:
            reply = UT5250_DONT;
            break;
            }
        break;

    case UT5250_DONT:
    case UT5250_WONT:
    default:
        return;
        }

    ultra_stream_append3(stream, UT5250_IAC, reply, what);
    }

static void ultra_stream_sb_var_value(Tn5250Buffer* buf, unsigned char* var,
        unsigned char* value)
    {
    tn5250_buffer_append_byte(buf, UT5250_VAR);
    tn5250_buffer_append_data(buf, var, (int)strlen((char*)var));
    tn5250_buffer_append_byte(buf, UT5250_VALUE);
    tn5250_buffer_append_data(buf, value, (int)strlen((char*)value));
    }

static void ultra_stream_sb(Tn5250Stream* stream, unsigned char* sb,
        int sb_len)
    {
    Tn5250Buffer out;

    if (sb_len <= 0)
        return;

    tn5250_buffer_init(&out);

    if (sb[0] == UT5250_TERMINAL_TYPE)
        {
        const char* termtype;

        if (sb_len < 2 || sb[1] != UT5250_SEND)
            {
            tn5250_buffer_free(&out);
            return;
            }

        termtype = tn5250_stream_getenv(stream, "TERM");
        if (termtype == NULL || termtype[0] == '\0')
            termtype = "IBM-3179-2";

        tn5250_buffer_append_byte(&out, UT5250_IAC);
        tn5250_buffer_append_byte(&out, UT5250_SB);
        tn5250_buffer_append_byte(&out, UT5250_TERMINAL_TYPE);
        tn5250_buffer_append_byte(&out, UT5250_IS);
        tn5250_buffer_append_data(&out, (unsigned char*)termtype,
                (int)strlen(termtype));
        tn5250_buffer_append_byte(&out, UT5250_IAC);
        tn5250_buffer_append_byte(&out, UT5250_SE);
        ultra_stream_append(stream, tn5250_buffer_data(&out),
                tn5250_buffer_length(&out));
        stream->status |= 1;
        }
    else if (sb[0] == UT5250_NEW_ENVIRON)
        {
        Tn5250ConfigStr* iter;

        tn5250_buffer_append_byte(&out, UT5250_IAC);
        tn5250_buffer_append_byte(&out, UT5250_SB);
        tn5250_buffer_append_byte(&out, UT5250_NEW_ENVIRON);
        tn5250_buffer_append_byte(&out, UT5250_IS);

        if (stream->config != NULL && stream->config->vars != NULL)
            {
            iter = stream->config->vars;
            do {
                if (strlen(iter->name) > 4 &&
                        memcmp(iter->name, "env.", 4) == 0)
                    {
                    ultra_stream_sb_var_value(&out,
                            (unsigned char*)iter->name + 4,
                            (unsigned char*)iter->value);
                    }
                iter = iter->next;
            } while (iter != stream->config->vars);
            }

        tn5250_buffer_append_byte(&out, UT5250_IAC);
        tn5250_buffer_append_byte(&out, UT5250_SE);
        ultra_stream_append(stream, tn5250_buffer_data(&out),
                tn5250_buffer_length(&out));
        }

    tn5250_buffer_free(&out);
    }

static void ultra_stream_send_packet(Tn5250Stream* stream, int length,
        StreamHeader header, unsigned char* data)
    {
    Tn5250Buffer out;
    int full_length;

    full_length = length + 10;
    tn5250_buffer_init(&out);
    tn5250_buffer_append_byte(&out, (unsigned char)(((short)full_length) >> 8));
    tn5250_buffer_append_byte(&out, (unsigned char)(full_length & 0xff));
    tn5250_buffer_append_byte(&out, 0x12);
    tn5250_buffer_append_byte(&out, 0xa0);
    tn5250_buffer_append_byte(&out, (unsigned char)(header.flowtype >> 8));
    tn5250_buffer_append_byte(&out, (unsigned char)(header.flowtype & 0xff));
    tn5250_buffer_append_byte(&out, 4);
    tn5250_buffer_append_byte(&out, header.flags);
    tn5250_buffer_append_byte(&out, 0);
    tn5250_buffer_append_byte(&out, header.opcode);
    if (length > 0 && data != NULL)
        tn5250_buffer_append_data(&out, data, length);

    ultra_stream_escape(&out);
    tn5250_buffer_append_byte(&out, UT5250_IAC);
    tn5250_buffer_append_byte(&out, UT5250_EOR);
    ultra_stream_append(stream, tn5250_buffer_data(&out),
            tn5250_buffer_length(&out));
    tn5250_buffer_free(&out);
    }

static void ultra_stream_escape(Tn5250Buffer* buffer)
    {
    Tn5250Buffer out;
    int i;
    unsigned char c;

    tn5250_buffer_init(&out);
    for (i = 0; i < tn5250_buffer_length(buffer); ++i)
        {
        c = tn5250_buffer_data(buffer)[i];
        tn5250_buffer_append_byte(&out, c);
        if (c == UT5250_IAC)
            tn5250_buffer_append_byte(&out, UT5250_IAC);
        }

    tn5250_buffer_free(buffer);
    memcpy(buffer, &out, sizeof(Tn5250Buffer));
    }

static int ultra_stream_append3(Tn5250Stream* stream, unsigned char a,
        unsigned char b, unsigned char c)
    {
    unsigned char buf[3];

    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    return ultra_stream_append(stream, buf, 3);
    }

static int ultra_stream_start_telnet(Tn5250Stream* stream)
    {
    UT5250_STREAM_STATE* state;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state == NULL)
        return -1;
    if (state->telnet_started)
        return 0;

    if (ultra_stream_append3(stream, UT5250_IAC, UT5250_WILL,
                UT5250_TERMINAL_TYPE) < 0 ||
            ultra_stream_append3(stream, UT5250_IAC, UT5250_WILL,
                UT5250_NEW_ENVIRON) < 0 ||
            ultra_stream_append3(stream, UT5250_IAC, UT5250_DO,
                UT5250_END_OF_RECORD) < 0 ||
            ultra_stream_append3(stream, UT5250_IAC, UT5250_WILL,
                UT5250_END_OF_RECORD) < 0 ||
            ultra_stream_append3(stream, UT5250_IAC, UT5250_DO,
                UT5250_TRANSMIT_BINARY) < 0 ||
            ultra_stream_append3(stream, UT5250_IAC, UT5250_WILL,
                UT5250_TRANSMIT_BINARY) < 0)
        return -1;

    state->telnet_started = 1;
    return 0;
    }

static int ultra_stream_append(Tn5250Stream* stream, const unsigned char* data,
        int len)
    {
#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
    UT5250_STREAM_STATE* state;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state && state->tls_requested && state->tls_handshake_done)
        return ultra_stream_tls_write(stream, data, len);
#endif

    return ultra_stream_append_raw(stream, data, len);
    }

static int ultra_stream_append_raw(Tn5250Stream* stream,
        const unsigned char* data, int len)
    {
    UT5250_STREAM_STATE* state;
    int copy_len;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state == NULL || data == NULL || len <= 0)
        return 0;

    if (state->out == NULL || state->out_len == NULL ||
            state->out_limit <= *state->out_len)
        return -1;

    copy_len = len;
    if (*state->out_len + copy_len > state->out_limit)
        copy_len = state->out_limit - *state->out_len;

    if (copy_len > 0)
        {
        memcpy(state->out + *state->out_len, data, copy_len);
        *state->out_len += copy_len;
        }

    return (copy_len == len) ? 0 : -1;
    }

static void ultra_stream_error(char* error, int error_len, const char* text)
    {
    if (error == NULL || error_len <= 0)
        return;
    if (text == NULL)
        text = "";
    strncpy(error, text, error_len - 1);
    error[error_len - 1] = '\0';
    }

#if defined(ULTRATERMINAL_TN5250_HAVE_OPENSSL)
static int ultra_stream_tls_drive(Tn5250Stream* stream)
    {
    UT5250_STREAM_STATE* state;
    int rc;
    int err;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state == NULL || state->tls_ssl == NULL)
        return -1;
    if (state->tls_handshake_done)
        return ultra_stream_tls_drain(stream);

    rc = SSL_do_handshake(state->tls_ssl);
    ultra_stream_tls_drain(stream);
    if (rc == 1)
        {
        state->tls_handshake_done = 1;
        return ultra_stream_start_telnet(stream);
        }

    err = SSL_get_error(state->tls_ssl, rc);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
        return 0;
    return -1;
    }

static int ultra_stream_tls_drain(Tn5250Stream* stream)
    {
    UT5250_STREAM_STATE* state;
    unsigned char tmp[2048];
    int n;
    int rc;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state == NULL || state->tls_wbio == NULL)
        return -1;

    rc = 0;
    while ((n = BIO_read(state->tls_wbio, tmp, sizeof(tmp))) > 0)
        {
        if (ultra_stream_append_raw(stream, tmp, n) < 0)
            rc = -1;
        }
    return rc;
    }

static int ultra_stream_tls_feed(Tn5250Stream* stream, const unsigned char* data,
        int data_len)
    {
    UT5250_STREAM_STATE* state;
    unsigned char plain[4096];
    int n;
    int err;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state == NULL || state->tls_ssl == NULL || state->tls_rbio == NULL)
        return 0;

    if (data_len > 0 && BIO_write(state->tls_rbio, data, data_len) <= 0)
        return 0;

    if (!state->tls_handshake_done)
        {
        if (ultra_stream_tls_drive(stream) < 0)
            return 0;
        if (!state->tls_handshake_done)
            return 1;
        }

    for (;;)
        {
        n = SSL_read(state->tls_ssl, plain, sizeof(plain));
        if (n > 0)
            {
            if (ultra_stream_tls_feed_plain(stream, plain, n) < 0)
                return 0;
            continue;
            }

        err = SSL_get_error(state->tls_ssl, n);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            break;
        if (err == SSL_ERROR_ZERO_RETURN)
            break;
        return 0;
        }

    ultra_stream_tls_drain(stream);
    return 1;
    }

static int ultra_stream_tls_feed_plain(Tn5250Stream* stream,
        const unsigned char* data, int len)
    {
    UT5250_STREAM_STATE* state;
    const unsigned char* old_in;
    int old_len;
    int old_pos;
    int rc;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    old_in = state->in;
    old_len = state->in_len;
    old_pos = state->in_pos;

    state->in = data;
    state->in_len = len;
    state->in_pos = 0;
    rc = tn5250_stream_handle_receive(stream);

    state->in = old_in;
    state->in_len = old_len;
    state->in_pos = old_pos;
    return rc ? 0 : -1;
    }

static int ultra_stream_tls_write(Tn5250Stream* stream,
        const unsigned char* data, int len)
    {
    UT5250_STREAM_STATE* state;
    int offset;
    int n;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state == NULL || state->tls_ssl == NULL || data == NULL || len <= 0)
        return 0;

    offset = 0;
    while (offset < len)
        {
        n = SSL_write(state->tls_ssl, data + offset, len - offset);
        if (n <= 0)
            {
            if (ultra_stream_tls_error(stream, n) == 0)
                continue;
            return -1;
            }
        offset += n;
        if (ultra_stream_tls_drain(stream) < 0)
            return -1;
        }
    return 0;
    }

static int ultra_stream_tls_error(Tn5250Stream* stream, int rc)
    {
    UT5250_STREAM_STATE* state;
    int err;

    state = (UT5250_STREAM_STATE*)stream->userdata;
    if (state == NULL || state->tls_ssl == NULL)
        return -1;
    err = SSL_get_error(state->tls_ssl, rc);
    return (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) ? 0 : -1;
    }
#endif
