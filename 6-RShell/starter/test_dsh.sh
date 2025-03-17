#!/bin/bash

# Test script for Remote Shell
# Usage: ./test_dsh.sh

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Function to check if a command failed
check_result() {
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}PASSED${NC}: $1"
    else
        echo -e "${RED}FAILED${NC}: $1"
        exit 1
    fi
}

echo "Building dsh..."
make clean && make
check_result "Build"

# Run server in background
echo "Starting server on port 8888..."
./dsh -s -p 8888 &
SERVER_PID=$!
sleep 1  # Give server time to start

# Test basic command
echo "Testing basic command execution..."
echo "ls" | ./dsh -c -p 8888 > /tmp/dsh_output.txt
grep "dsh" /tmp/dsh_output.txt > /dev/null
check_result "Basic command execution"

# Test built-in command
echo "Testing built-in cd command..."
echo -e "cd /tmp\npwd" | ./dsh -c -p 8888 > /tmp/dsh_output.txt
grep "tmp" /tmp/dsh_output.txt > /dev/null
check_result "Built-in cd command"

# Test piped command
echo "Testing pipe command..."
echo "ls | grep .c" | ./dsh -c -p 8888 > /tmp/dsh_output.txt
grep ".c" /tmp/dsh_output.txt > /dev/null
check_result "Pipe command"

# Cleanup
echo "Stopping server..."
echo "stop-server" | ./dsh -c -p 8888 > /dev/null
sleep 1

# In case server didn't stop
if kill -0 $SERVER_PID 2>/dev/null; then
    kill $SERVER_PID
fi

echo "Tests completed successfully!"
