/*
 * humatric_protocol.h
 * * Serial communication protocol definitions for Humatric's pad controller system.
 * * This file defines the command codes, frame structures and message formats
 * used for communicating between a PC and the Humatric controller over serial.
 * * Copyright (c) 2025 Giuseppe Massimo Bertani
 * All rights reserved.
 * * Unauthorized copying of this file, via any medium is strictly prohibited.
 * Proprietary and confidential.
 */

#ifndef HUMATRIC_PROTOCOL_H
#define HUMATRIC_PROTOCOL_H

#ifdef _MSC_VER
#define PACKED_STRUCT_BEGIN __pragma(pack(push, 1))
#define PACKED_STRUCT_END   __pragma(pack(pop))
#else
#define PACKED_STRUCT_BEGIN
#define PACKED_STRUCT_END   __attribute__((packed))
#endif

#include <stdint.h>
#include <stddef.h>

#ifndef MAX_SSID_LEN
#define MAX_SSID_LEN        32
#endif

#define CMD_HEADER_MARKER   0xFEED
#define RSP_HEADER_MARKER   0xBEEF
#define BROADCAST_MASK      0xFFFF
#define MAX_PASS_LEN        64
#define MAX_SERIAL_PARAM    20
#define MAX_FW_VER_LEN      15
#define MAX_SERIAL_ID_LEN   20
#define PROTOCOL_EOT        0x04

// Enumeration of all command codes
typedef enum {
    CMD_START_STREAM       = 0x01,
    CMD_START_ACQ          = 0x02,
    CMD_STOP_ACQ           = 0x03,
    CMD_CLEAR_CACHE        = 0x04,
    CMD_STOP_STREAM        = 0x05,
    CMD_GET_FRAME          = 0x06,
    CMD_SET_SAMPLING_RATE  = 0x07,
    CMD_GET_SAMPLING_RATE  = 0x08,
    CMD_SET_CALIBRATION    = 0x11,
    CMD_SET_CHANNEL_MASK   = 0x12,
    CMD_GET_CHANNEL_MASK   = 0x13,
    CMD_RUN_SELF_TEST      = 0x21,
    CMD_GET_STATUS         = 0x22,
    CMD_GET_FW_VERSION     = 0x23,
    CMD_GET_SERIAL_NUMBER  = 0x24,
    CMD_SET_SERIAL_PARAM   = 0x31,
    CMD_SET_WIFI_SSID      = 0x32
} ECommandCode;

// Command envelope (PC → controller)
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;
    uint16_t padMask;
    uint8_t  commandCode;
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_CommandBase;
PACKED_STRUCT_END;

// Frame sent from pads (real-time or cached data)
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;       // 0xBEEF
    uint8_t  padAddress;   // 1..16
    uint8_t  commandCode;  // CMD_START_STREAM or CMD_GET_FRAME
    uint32_t timestamp;    // ms since first frame (or seconds from Jan 1, 2025)
    int16_t  forceX;
    int16_t  forceY;
    int16_t  forceZ;
    int16_t  momentX;
    int16_t  momentY;
    int16_t  momentZ;
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_Frame;
PACKED_STRUCT_END;

// ACK response (controller → PC)
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;       // 0xBEEF
    uint8_t  padAddress;   // 0 or specific pad
    uint8_t  commandCode;  // echoed command
    uint8_t  ackCode;      // 0x06
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_AckResponse;
PACKED_STRUCT_END;

// NACK response (controller → PC)
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;       // 0xBEEF
    uint8_t  padAddress;   // 0 or specific pad
    uint8_t  commandCode;  // that caused error
    uint8_t  nakCode;      // 0x15
    uint16_t errorCode;
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_NackResponse;
PACKED_STRUCT_END;

// GET_SAMPLE_RATE response
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;       // 0xBEEF
    uint8_t  padAddress;   // 0 or specific pad
    uint8_t  commandCode;  // 0x08 GET_SAMPLE_RATE
    uint16_t  sampleRate;
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_SampleRateResponse;
PACKED_STRUCT_END;


// GET_CHANNEL_MASK response
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;
    uint8_t  padAddress;
    uint8_t  commandCode;
    uint8_t  channelMask;
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_ChannelMaskResponse;
PACKED_STRUCT_END;

// GET_STATUS response (general or per-pad)
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;
    uint8_t  padAddress;                         // 0 = general, else pad ID
    uint8_t  commandCode;                        // CMD_GET_STATUS
    uint32_t errorFlags;
    int8_t   temperature;                        // signed °C
    char     serialParams[MAX_SERIAL_PARAM];     // "115200,8,n,1"
    char     wifiSSID[MAX_SSID_LEN];             // SSID or zero-filled
    uint8_t  wifiIP[4];                          // IP address
    uint8_t  wifiMask[4];                        // subnet mask
    uint8_t  wifiGW[4];                          // gateway
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_StatusResponse;
PACKED_STRUCT_END;

// GET_FW_VERSION response
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;
    uint8_t  padAddress;
    uint8_t  commandCode;
    char     stm32FW[MAX_FW_VER_LEN];            // "001.002.003"
    char     esp32FW[MAX_FW_VER_LEN];            // "001.002.003"
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_FirmwareVersionResponse;
PACKED_STRUCT_END;

// GET_SERIAL_NUMBER response
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;
    uint8_t  padAddress;
    uint8_t  commandCode;
    char     serialID[MAX_SERIAL_ID_LEN];        // null-terminated string
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_SerialNumberResponse;
PACKED_STRUCT_END;

// NOTIFY message (only sent during active streaming)
// Similar to ACK structure, but commandCode is zero (unsolicited message),
// and notifyCode ranges from 0xF0 (NOTIFY_0) to 0xF7 (NOTIFY_7).
PACKED_STRUCT_BEGIN
    typedef struct {
    uint16_t header;       // 0xBEEF
    uint8_t  padAddress;   // 1..16
    uint8_t  commandCode;  // always 0x00
    uint8_t  notifyCode;   // 0xF0 - 0xF7
    uint16_t crc;
    uint8_t  eot1;
    uint8_t  eot2;
} T_NotifyMessage;
PACKED_STRUCT_END;

uint16_t crc16(const uint8_t *data, size_t length);

#endif // HUMATRIC_PROTOCOL_H
