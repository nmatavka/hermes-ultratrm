/*
 * Focused TN3270/TN3270E protocol definitions adapted from suite3270 4.5.
 * See LICENSE.md in this directory for the BSD-style notice.
 */

#if !defined(ULTRATERMINAL_TN3270_PROTOCOL_H)
#define ULTRATERMINAL_TN3270_PROTOCOL_H

#define UT3270_CMD_W        0x01
#define UT3270_CMD_RB       0x02
#define UT3270_CMD_NOP      0x03
#define UT3270_CMD_EW       0x05
#define UT3270_CMD_RM       0x06
#define UT3270_CMD_EWA      0x0d
#define UT3270_CMD_RMA      0x0e
#define UT3270_CMD_EAU      0x0f
#define UT3270_CMD_WSF      0x11

#define UT3270_SNA_CMD_RMA  0x6e
#define UT3270_SNA_CMD_EAU  0x6f
#define UT3270_SNA_CMD_EWA  0x7e
#define UT3270_SNA_CMD_W    0xf1
#define UT3270_SNA_CMD_RB   0xf2
#define UT3270_SNA_CMD_WSF  0xf3
#define UT3270_SNA_CMD_EW   0xf5
#define UT3270_SNA_CMD_RM   0xf6

#define UT3270_ORDER_PT     0x05
#define UT3270_ORDER_GE     0x08
#define UT3270_ORDER_SBA    0x11
#define UT3270_ORDER_EUA    0x12
#define UT3270_ORDER_IC     0x13
#define UT3270_ORDER_SF     0x1d
#define UT3270_ORDER_SA     0x28
#define UT3270_ORDER_SFE    0x29
#define UT3270_ORDER_YALE   0x2b
#define UT3270_ORDER_MF     0x2c
#define UT3270_ORDER_RA     0x3c

#define UT3270_FC_NULL      0x00
#define UT3270_FC_FF        0x0c
#define UT3270_FC_CR        0x0d
#define UT3270_FC_SO        0x0e
#define UT3270_FC_SI        0x0f
#define UT3270_FC_NL        0x15
#define UT3270_FC_EM        0x19
#define UT3270_FC_DUP       0x1c
#define UT3270_FC_FM        0x1e
#define UT3270_FC_LF        0x25
#define UT3270_FC_SUB       0x3f
#define UT3270_FC_EO        0xff

#define UT3270_SCS_BS       0x16
#define UT3270_SCS_BEL      0x2f
#define UT3270_SCS_CR       0x0d
#define UT3270_SCS_ENP      0x14
#define UT3270_SCS_FF       0x0c
#define UT3270_SCS_GE       0x08
#define UT3270_SCS_HT       0x05
#define UT3270_SCS_INP      0x24
#define UT3270_SCS_IRS      0x1e
#define UT3270_SCS_LF       0x25
#define UT3270_SCS_NL       0x15
#define UT3270_SCS_SA       0x28
#define UT3270_SCS_SET      0x2b
#define UT3270_SCS_SO       0x0e
#define UT3270_SCS_SI       0x0f
#define UT3270_SCS_TRN      0x35
#define UT3270_SCS_VCS      0x04
#define UT3270_SCS_VT       0x0b

#define UT3270_SF_READ_PART        0x01
#define UT3270_SF_RP_QUERY         0x02
#define UT3270_SF_RP_QLIST         0x03
#define UT3270_SF_RPQ_LIST         0x00
#define UT3270_SF_RPQ_EQUIV        0x40
#define UT3270_SF_RPQ_ALL          0x80
#define UT3270_SF_ERASE_RESET      0x03
#define UT3270_SF_ER_DEFAULT       0x00
#define UT3270_SF_ER_ALT           0x80
#define UT3270_SF_SET_REPLY_MODE   0x09
#define UT3270_SF_SRM_FIELD        0x00
#define UT3270_SF_SRM_XFIELD       0x01
#define UT3270_SF_SRM_CHAR         0x02
#define UT3270_SF_CREATE_PART      0x0c
#define UT3270_SF_OUTBOUND_DS      0x40
#define UT3270_SF_TRANSFER_DATA    0xd0

#define UT3270_QR_SUMMARY          0x80
#define UT3270_QR_USABLE_AREA      0x81
#define UT3270_QR_IMAGE            0x82
#define UT3270_QR_TEXT_PART        0x83
#define UT3270_QR_ALPHA_PART       0x84
#define UT3270_QR_CHARSETS         0x85
#define UT3270_QR_COLOR            0x86
#define UT3270_QR_HIGHLIGHTING     0x87
#define UT3270_QR_REPLY_MODES      0x88
#define UT3270_QR_FIELD_VAL        0x8a
#define UT3270_QR_OUTLINING        0x8c
#define UT3270_QR_EAST_ASIA        0x91
#define UT3270_QR_IMP_PART         0xa6
#define UT3270_QR_NULL             0xff

#define UT3270_WCC_RESET_BIT             0x40
#define UT3270_WCC_START_PRINTER_BIT     0x08
#define UT3270_WCC_SOUND_ALARM_BIT       0x04
#define UT3270_WCC_KEYBOARD_RESTORE_BIT  0x02
#define UT3270_WCC_RESET_MDT_BIT       0x01

#define UT3270_FA_PROTECT      0x20
#define UT3270_FA_NUMERIC      0x10
#define UT3270_FA_INTENSITY    0x0c
#define UT3270_FA_INT_NORM_SEL 0x04
#define UT3270_FA_INT_HIGH_SEL 0x08
#define UT3270_FA_INT_ZERO_NSEL 0x0c
#define UT3270_FA_MODIFY       0x01
#define UT3270_FA_MASK         (UT3270_FA_PROTECT | UT3270_FA_NUMERIC | UT3270_FA_INTENSITY | UT3270_FA_MODIFY)

#define UT3270_FA_IS_PROTECTED(c)  ((c) & UT3270_FA_PROTECT)
#define UT3270_FA_IS_MODIFIED(c)   ((c) & UT3270_FA_MODIFY)
#define UT3270_FA_IS_NUMERIC(c)    ((c) & UT3270_FA_NUMERIC)
#define UT3270_FA_IS_SKIP(c)       (((c) & UT3270_FA_PROTECT) && ((c) & UT3270_FA_NUMERIC))
#define UT3270_FA_IS_ZERO(c)       (((c) & UT3270_FA_INTENSITY) == UT3270_FA_INT_ZERO_NSEL)
#define UT3270_FA_IS_HIGH(c)       (((c) & UT3270_FA_INTENSITY) == UT3270_FA_INT_HIGH_SEL)

#define UT3270_XA_ALL          0x00
#define UT3270_XA_3270         0xc0
#define UT3270_XA_VALIDATION   0xc1
#define UT3270_XA_OUTLINING    0xc2
#define UT3270_XA_HIGHLIGHTING 0x41
#define UT3270_XA_FOREGROUND   0x42
#define UT3270_XA_CHARSET      0x43
#define UT3270_XA_BACKGROUND   0x45
#define UT3270_XA_TRANSPARENCY 0x46
#define UT3270_XA_INPUT_CONTROL 0xfe
#define UT3270_XAI_DISABLED    0x00
#define UT3270_XAI_ENABLED     0x01
#define UT3270_XAC_DEFAULT     0x00
#define UT3270_XAH_DEFAULT     0x00
#define UT3270_XAH_NORMAL      0xf0
#define UT3270_XAH_BLINK       0xf1
#define UT3270_XAH_REVERSE     0xf2
#define UT3270_XAH_UNDERSCORE  0xf4
#define UT3270_XAH_INTENSIFY   0xf8

#define UT3270_AID_NO      0x60
#define UT3270_AID_QREPLY  0x61
#define UT3270_AID_ENTER   0x7d
#define UT3270_AID_PF1     0xf1
#define UT3270_AID_PF2     0xf2
#define UT3270_AID_PF3     0xf3
#define UT3270_AID_PF4     0xf4
#define UT3270_AID_PF5     0xf5
#define UT3270_AID_PF6     0xf6
#define UT3270_AID_PF7     0xf7
#define UT3270_AID_PF8     0xf8
#define UT3270_AID_PF9     0xf9
#define UT3270_AID_PF10    0x7a
#define UT3270_AID_PF11    0x7b
#define UT3270_AID_PF12    0x7c
#define UT3270_AID_PF13    0xc1
#define UT3270_AID_PF14    0xc2
#define UT3270_AID_PF15    0xc3
#define UT3270_AID_PF16    0xc4
#define UT3270_AID_PF17    0xc5
#define UT3270_AID_PF18    0xc6
#define UT3270_AID_PF19    0xc7
#define UT3270_AID_PF20    0xc8
#define UT3270_AID_PF21    0xc9
#define UT3270_AID_PF22    0x4a
#define UT3270_AID_PF23    0x4b
#define UT3270_AID_PF24    0x4c
#define UT3270_AID_PA1     0x6c
#define UT3270_AID_PA2     0x6e
#define UT3270_AID_PA3     0x6b
#define UT3270_AID_CLEAR   0x6d
#define UT3270_AID_SYSREQ  0xf0
#define UT3270_AID_SF      0x88
#define UT3270_SFID_QREPLY 0x81

#define UT_TN3270E_OP_ASSOCIATE    0
#define UT_TN3270E_OP_CONNECT      1
#define UT_TN3270E_OP_DEVICE_TYPE  2
#define UT_TN3270E_OP_FUNCTIONS    3
#define UT_TN3270E_OP_IS           4
#define UT_TN3270E_OP_REASON       5
#define UT_TN3270E_OP_REJECT       6
#define UT_TN3270E_OP_REQUEST      7
#define UT_TN3270E_OP_SEND         8

#define UT_TN3270E_FUNC_BIND_IMAGE             0
#define UT_TN3270E_FUNC_DATA_STREAM_CTL        1
#define UT_TN3270E_FUNC_RESPONSES              2
#define UT_TN3270E_FUNC_SCS_CTL_CODES          3
#define UT_TN3270E_FUNC_SYSREQ                 4
#define UT_TN3270E_FUNC_CONTENTION_RESOLUTION  5
#define UT_TN3270E_FUNC_SNA_SENSE              6

#define UT_TN3270E_DT_3270_DATA     0x00
#define UT_TN3270E_DT_SCS_DATA      0x01
#define UT_TN3270E_DT_RESPONSE      0x02
#define UT_TN3270E_DT_BIND_IMAGE    0x03
#define UT_TN3270E_DT_UNBIND        0x04
#define UT_TN3270E_DT_NVT_DATA      0x05
#define UT_TN3270E_DT_REQUEST       0x06
#define UT_TN3270E_DT_SSCP_LU_DATA  0x07
#define UT_TN3270E_DT_PRINT_EOJ     0x08
#define UT_TN3270E_DT_BID           0x09

#define UT_TN3270E_RQF_SEND_DATA    0x01
#define UT_TN3270E_RQF_KEYBOARD_RESTORE 0x02
#define UT_TN3270E_RQF_SIGNAL       0x04
#define UT_TN3270E_RSF_NO_RESPONSE  0x00
#define UT_TN3270E_RSF_ERROR_RESPONSE 0x01
#define UT_TN3270E_RSF_ALWAYS_RESPONSE 0x02
#define UT_TN3270E_RSF_POSITIVE_RESPONSE 0x00
#define UT_TN3270E_RSF_NEGATIVE_RESPONSE 0x01
#define UT_TN3270E_RSF_SNA_SENSE    0x02
#define UT_TN3270E_POS_DEVICE_END   0x00
#define UT_TN3270E_NEG_COMMAND_REJECT 0x00
#define UT_TN3270E_EH_SIZE          5

#define UT_TN3270E_UNBIND_NORMAL    0x01
#define UT_TN3270E_UNBIND_BIND_FORTHCOMING 0x02

#define UT3270_COLOR_DEFAULT        0x00
#define UT3270_COLOR_NEUTRAL_BLACK  0xf0
#define UT3270_COLOR_BLUE           0xf1
#define UT3270_COLOR_RED            0xf2
#define UT3270_COLOR_PINK           0xf3
#define UT3270_COLOR_GREEN          0xf4
#define UT3270_COLOR_TURQUOISE      0xf5
#define UT3270_COLOR_YELLOW         0xf6
#define UT3270_COLOR_NEUTRAL_WHITE  0xf7
#define UT3270_COLOR_BLACK          0xf8
#define UT3270_COLOR_DEEP_BLUE      0xf9
#define UT3270_COLOR_ORANGE         0xfa
#define UT3270_COLOR_PURPLE         0xfb
#define UT3270_COLOR_PALE_GREEN     0xfc
#define UT3270_COLOR_PALE_TURQUOISE 0xfd
#define UT3270_COLOR_GREY           0xfe
#define UT3270_COLOR_WHITE          0xff

#define UT3270_BIND_RU              0x31
#define UT3270_BIND_OFF_MAXRU_SEC   10
#define UT3270_BIND_OFF_MAXRU_PRI   11
#define UT3270_BIND_OFF_RD          20
#define UT3270_BIND_OFF_CD          21
#define UT3270_BIND_OFF_RA          22
#define UT3270_BIND_OFF_CA          23
#define UT3270_BIND_OFF_SSIZE       24
#define UT3270_BIND_OFF_PLU_NAME_LEN 27
#define UT3270_BIND_PLU_NAME_MAX    8
#define UT3270_BIND_OFF_PLU_NAME    28

int ut3270_decode_baddr(unsigned char c1, unsigned char c2);
void ut3270_encode_baddr(unsigned char *out, int addr, int rows, int cols);
unsigned char ut3270_address_code(int value);

#endif
