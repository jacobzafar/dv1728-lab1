#ifndef CALCLIB_H
#define CALCLIB_H

#include <stdint.h>

// Function to perform arithmetic operations
int32_t calculate(uint32_t operation, int32_t value1, int32_t value2);

// Function to convert operation string to operation code
uint32_t string_to_operation(const char* op_str);

// Function to convert operation code to string
const char* operation_to_string(uint32_t operation);

#endif // CALCLIB_H
