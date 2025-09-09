#!/bin/bash

# Simple test script to verify client functionality
# Note: This requires the client to be built and servers to be accessible

echo "Testing client functionality..."

# Test URL parsing with invalid formats
echo "Testing invalid URL format..."
./client invalid_url 2>/dev/null
if [ $? -eq 1 ]; then
    echo "✓ Invalid URL properly rejected"
else
    echo "✗ Invalid URL not properly rejected"
fi

# Test with invalid protocol
echo "Testing invalid protocol..."
./client xyz://example.com:5000/text 2>/dev/null
if [ $? -eq 1 ]; then
    echo "✓ Invalid protocol properly rejected"
else
    echo "✗ Invalid protocol not properly rejected"
fi

# Test with invalid API
echo "Testing invalid API..."
./client tcp://example.com:5000/invalidapi 2>/dev/null
if [ $? -eq 1 ]; then
    echo "✓ Invalid API properly rejected"
else
    echo "✗ Invalid API not properly rejected"
fi

echo ""
echo "Basic parsing tests completed."
echo ""
echo "To test network functionality, run:"
echo "  ./client tcp://alice.nplab.bth.se:5000/text"
echo "  ./client udp://bob.nplab.bth.se:5000/binary"
echo "  ./client any://bob.nplab.bth.se:5000/text"
