#include "calcLib.h"
#include "protocol.h"
#include <string.h>

#ifdef _WIN32
    #define strcasecmp _stricmp
#else
    #include <strings.h>
#endif

int32_t calculate(uint32_t operation, int32_t value1, int32_t value2) {
    switch (operation) {
        case ARITH_ADD:
            return value1 + value2;
        case ARITH_SUB:
            return value1 - value2;
        case ARITH_MUL:
            return value1 * value2;
        case ARITH_DIV:
            if (value2 == 0) {
                return 0; // Handle division by zero
            }
            return value1 / value2; // Integer division (truncated)
        default:
            return 0;
    }
}

uint32_t string_to_operation(const char* op_str) {
    if (strcasecmp(op_str, "add") == 0) {
        return ARITH_ADD;
    } else if (strcasecmp(op_str, "sub") == 0) {
        return ARITH_SUB;
    } else if (strcasecmp(op_str, "mul") == 0) {
        return ARITH_MUL;
    } else if (strcasecmp(op_str, "div") == 0) {
        return ARITH_DIV;
    }
    return 0; // Unknown operation
}

const char* operation_to_string(uint32_t operation) {
    switch (operation) {
        case ARITH_ADD:
            return "add";
        case ARITH_SUB:
            return "sub";
        case ARITH_MUL:
            return "mul";
        case ARITH_DIV:
            return "div";
        default:
            return "unknown";
    }
}
