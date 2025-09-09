#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

// Protocol version
#define MAJOR_VERSION 1
#define MINOR_VERSION 1

// Message types
#define MSG_TYPE_CALC_MESSAGE 22
#define MSG_TYPE_CALC_PROTOCOL 1
#define MSG_TYPE_NOT_OK 2

// Protocol ID
#define PROTOCOL_UDP 17

// Arithmetic operations
#define ARITH_ADD 1
#define ARITH_SUB 2
#define ARITH_MUL 3
#define ARITH_DIV 4

// Message structure for initial client message and server responses
typedef struct {
    uint16_t type;           // Message type
    uint16_t message;        // Message code (0 for initial, 1 for OK, 2 for NOT OK)
    uint16_t protocol;       // Protocol ID (17 for UDP)
    uint16_t major_version;  // Major version
    uint16_t minor_version;  // Minor version
} __attribute__((packed)) calcMessage;

// Message structure for server arithmetic assignment
typedef struct {
    uint16_t type;           // Message type (1 for calcProtocol)
    uint16_t major_version;  // Major version
    uint16_t minor_version;  // Minor version
    uint32_t id;            // Message ID
    uint32_t arith;         // Arithmetic operation
    int32_t inValue1;       // First operand
    int32_t inValue2;       // Second operand
    int32_t inResult;       // Result (filled by client)
} __attribute__((packed)) calcProtocol;

#endif // PROTOCOL_H
