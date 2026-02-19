#!/bin/bash

set -e # Exit immediately if any command fails, which is important for testing.

echo "Starting analyzer tests..."
echo "--------------------------"

# --- Test 1: Uppercaser and Logger ---
# Expected: [logger] HELLO
echo "Running Test 1: uppercaser logger"

# Run the analyzer and capture the whole output, silencing errors (2>/dev/null)
OUTPUT1=$(echo -e "hello\n<END>" | ./output/analyzer 10 uppercaser logger 2>/dev/null)

# Use grep to pull out the line we care about (the logger output)
ACTUAL1=$(echo "$OUTPUT1" | grep "\[logger\]")

if [ "$ACTUAL1" = "[logger] HELLO" ]; then
    echo "Test 1: PASS 👍"
else
    echo "Test 1: FAIL ❌ (Expected: [logger] HELLO, Got: $ACTUAL1)"
    echo "Full Output for debug: $OUTPUT1"
fi
echo ""

# --- Test 2: Typewriter ---
# Expected: Just check if 'hello' is in the output (since typewriter prints directly)
echo "Running Test 2: typewriter"

OUTPUT2=$(echo -e "hello\n<END>" | ./output/analyzer 10 typewriter 2>/dev/null)

# Use grep to check for the string. '-q' means quiet, it just checks the return code.
if echo "$OUTPUT2" | grep -q "hello"; then
    echo "Test 2: PASS 👍"
else
    echo "Test 2: FAIL ❌ (Did not find 'hello' in output)"
    echo "Full Output for debug: $OUTPUT2"
fi
echo ""

# --- Test 3: Uppercaser, Rotator, Logger ---
# Expected: [logger] OHELL (hello -> HELLO -> rotate right)
echo "Running Test 3: uppercaser rotator logger"

OUTPUT3=$(echo -e "hello\n<END>" | ./output/analyzer 10 uppercaser rotator logger 2>/dev/null)
ACTUAL3=$(echo "$OUTPUT3" | grep "\[logger\]")

if [ "$ACTUAL3" = "[logger] OHELL" ]; then
    echo "Test 3: PASS 👍"
else
    echo "Test 3: FAIL ❌ (Expected: [logger] OHELL, Got: $ACTUAL3)"
    echo "Full Output for debug: $OUTPUT3"
fi
echo ""

# --- Test 4: Flipper and Logger (Reverse) ---
# Expected: [logger] olleh
echo "Running Test 4: flipper logger"

OUTPUT4=$(echo -e "hello\n<END>" | ./output/analyzer 10 flipper logger 2>/dev/null)
ACTUAL4=$(echo "$OUTPUT4" | grep "\[logger\]")

if [ "$ACTUAL4" = "[logger] olleh" ]; then
    echo "Test 4: PASS 👍"
else
    echo "Test 4: FAIL ❌ (Expected: [logger] olleh, Got: $ACTUAL4)"
    echo "Full Output for debug: $OUTPUT4"
fi
echo ""

# --- Test 5: Expander and Logger (Space Separation) ---
# Expected: [logger] h e l l o
echo "Running Test 5: expander logger"

OUTPUT5=$(echo -e "hello\n<END>" | ./output/analyzer 10 expander logger 2>/dev/null)
ACTUAL5=$(echo "$OUTPUT5" | grep "\[logger\]")

if [ "$ACTUAL5" = "[logger] h e l l o" ]; then
    echo "Test 5: PASS 👍"
else
    echo "Test 5: FAIL ❌ (Expected: [logger] h e l l o, Got: $ACTUAL5)"
    echo "Full Output for debug: $OUTPUT5"
fi
echo ""

# --- Test 6: Invalid Plugin (Negative Test) ---
# Expected: The analyzer program should exit with an error code (non-zero status).
echo "Running Test 6: Invalid plugin check"

# The '!' reverses the exit code. If it fails, the 'if' is true.
if ! ./output/analyzer 10 invalid 2>/dev/null; then
    echo "Test 6: PASS 👍 (Program failed as expected on invalid plugin)"
else
    echo "Test 6: FAIL ❌ (Program exited successfully, which is wrong for 'invalid')"
fi
echo ""

# --- Test 7: Empty Input ---
# Expected: No output, or a shutdown message.
echo "Running Test 7: Empty input check"

OUTPUT7=$(echo -e "\n<END>" | ./output/analyzer 10 logger 2>/dev/null)

# Check if it's an empty string (-z) OR if it contains the shutdown message (-q is quiet grep)
if [ -z "$OUTPUT7" ] || echo "$OUTPUT7" | grep -q "Pipeline shutdown complete"; then
    echo "Test 7: PASS 👍 (Handled empty input correctly)"
else
    echo "Test 7: FAIL ❌ (Unexpected output on empty input)"
    echo "Full Output for debug: $OUTPUT7"
fi
echo ""

# --- Test 8 (Bonus): Double Uppercaser ---
# Expected: [logger] HELLO (Running uppercaser twice should be the same as once)
echo "Running Test 8 (Bonus): double uppercaser logger"

OUTPUT8=$(echo -e "hello\n<END>" | ./output/analyzer 10 uppercaser uppercaser logger 2>/dev/null)
ACTUAL8=$(echo "$OUTPUT8" | grep "\[logger\]")

if [ "$ACTUAL8" = "[logger] HELLO" ]; then
    echo "Test 8 (Bonus): PASS 👍"
else
    echo "Test 8 (Bonus): FAIL ❌ (Expected: [logger] HELLO, Got: $ACTUAL8)"
    echo "Full Output for debug: $OUTPUT8"
fi
echo ""

echo "--------------------------"
echo "Tests complete."