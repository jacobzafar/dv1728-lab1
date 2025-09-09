#include <iostream>
#include <string>
#include <cassert>
#include "calcLib.h"
#include "protocol.h"

// Test function prototypes
void testCalculations();
void testStringOperations();
void testProtocolStructures();

int main() {
    std::cout << "Running client functionality tests..." << std::endl;
    
    try {
        testCalculations();
        testStringOperations();
        testProtocolStructures();
        
        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}

void testCalculations() {
    std::cout << "Testing arithmetic calculations..." << std::endl;
    
    // Test addition
    assert(calculate(ARITH_ADD, 5, 3) == 8);
    assert(calculate(ARITH_ADD, -5, 3) == -2);
    
    // Test subtraction
    assert(calculate(ARITH_SUB, 10, 3) == 7);
    assert(calculate(ARITH_SUB, 3, 10) == -7);
    
    // Test multiplication
    assert(calculate(ARITH_MUL, 4, 3) == 12);
    assert(calculate(ARITH_MUL, -4, 3) == -12);
    
    // Test division
    assert(calculate(ARITH_DIV, 12, 3) == 4);
    assert(calculate(ARITH_DIV, 13, 3) == 4); // Integer division
    assert(calculate(ARITH_DIV, 10, 0) == 0); // Division by zero
    
    std::cout << "Arithmetic calculations: PASSED" << std::endl;
}

void testStringOperations() {
    std::cout << "Testing string to operation conversion..." << std::endl;
    
    // Test string to operation
    assert(string_to_operation("add") == ARITH_ADD);
    assert(string_to_operation("ADD") == ARITH_ADD);
    assert(string_to_operation("sub") == ARITH_SUB);
    assert(string_to_operation("SUB") == ARITH_SUB);
    assert(string_to_operation("mul") == ARITH_MUL);
    assert(string_to_operation("MUL") == ARITH_MUL);
    assert(string_to_operation("div") == ARITH_DIV);
    assert(string_to_operation("DIV") == ARITH_DIV);
    assert(string_to_operation("unknown") == 0);
    
    // Test operation to string
    assert(std::string(operation_to_string(ARITH_ADD)) == "add");
    assert(std::string(operation_to_string(ARITH_SUB)) == "sub");
    assert(std::string(operation_to_string(ARITH_MUL)) == "mul");
    assert(std::string(operation_to_string(ARITH_DIV)) == "div");
    assert(std::string(operation_to_string(99)) == "unknown");
    
    std::cout << "String operations: PASSED" << std::endl;
}

void testProtocolStructures() {
    std::cout << "Testing protocol structure sizes..." << std::endl;
    
    // Test structure sizes
    assert(sizeof(calcMessage) == 10); // 5 uint16_t fields
    assert(sizeof(calcProtocol) == 22); // As per protocol specification
    
    // Test that structures are properly packed
    calcMessage msg;
    msg.type = 0x1234;
    msg.message = 0x5678;
    msg.protocol = 0x9ABC;
    msg.major_version = 0xDEF0;
    msg.minor_version = 0x1122;
    
    // Verify no padding issues
    uint8_t* bytes = (uint8_t*)&msg;
    // This is just checking that we can access the memory
    (void)bytes; // Suppress unused variable warning
    
    std::cout << "Protocol structures: PASSED" << std::endl;
}
