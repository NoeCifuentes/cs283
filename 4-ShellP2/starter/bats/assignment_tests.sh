#!/usr/bin/env bats

############################ DO NOT EDIT THIS FILE #####################################
# File: assignment_tests.sh
#
# DO NOT EDIT THIS FILE
#
# Add/Edit Student tests in student_tests.sh
#
# All tests in this file must pass - it is used as part of grading!
########################################################################################

@test "Which which ... which?" {
    run "$PWD/dsh" <<EOF
which which
EOF

    # Strip all whitespace (spaces, tabs, newlines) from the output
    stripped_output=$(echo "$output" | tr -d '[:space:]')

    # Expected output with all whitespace removed for easier matching
    expected_output="/usr/bin/whichdsh2>dsh2>cmdloopreturned0"

    # These echo commands will help with debugging and will only print
    # if the test fails
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"

    # Check exact match
    [ "$stripped_output" = "$expected_output" ]
}

@test "It handles quoted spaces" {
    run "$PWD/dsh" <<EOF
   echo " hello     world     "
EOF

    # Strip all whitespace (spaces, tabs, newlines) from the output
    stripped_output=$(echo "$output" | tr -d '\t\n\r\f\v')

    # Expected output with all whitespace removed for easier matching
    expected_output=" hello     world     dsh2> dsh2> cmd loop returned 0"

    # These echo commands will help with debugging and will only print
    # if the test fails
    echo "Captured stdout:"
    echo "Output: $output"
    echo "Exit Status: $status"
    echo "${stripped_output} -> ${expected_output}"

    # Check exact match
    [ "$stripped_output" = "$expected_output" ]
}

