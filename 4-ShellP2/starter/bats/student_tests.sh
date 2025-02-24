#!/usr/bin/env bats

setup() {
    # Ensure we are using the correct shell binary
    test_shell="$PWD/dsh"
}

@test "Built-in: cd with valid directory" {
    run bash -c "mkdir -p testdir && cd testdir && pwd"
    expected_output=$(pwd)/testdir

    run bash -c "$test_shell && cd testdir && pwd"
    [ "$output" = "$expected_output" ]

    # Cleanup
    rmdir testdir
}

@test "Built-in: cd with no arguments" {
    run $test_shell <<EOF
cd
pwd
EOF
    # Expect the working directory to remain unchanged
    [ "$output" = "$(pwd)" ]
}

@test "Built-in: cd with invalid directory" {
    run $test_shell <<EOF
cd /doesnotexist
EOF
    [ "$status" -ne 0 ]  # Expect failure
}

@test "Built-in: exit command" {
    run $test_shell <<EOF
exit
EOF
    [ "$status" -eq 0 ]
}

@test "External: ls command" {
    run $test_shell <<EOF
ls
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh"* ]]  # Expect shell binary to be in output
}

@test "External: echo command" {
    run $test_shell <<EOF
echo Hello, World!
EOF
    [ "$output" = "Hello, World!" ]
}

@test "External: uname command" {
    run $test_shell <<EOF
uname
EOF
    [ "$status" -eq 0 ]
}

@test "Quoted arguments" {
    run $test_shell <<EOF
echo "Hello,      World"
EOF
    [ "$output" = "Hello,      World" ]  # Spaces inside quotes should be preserved
}

@test "Command with multiple arguments" {
    run $test_shell <<EOF
ls -l /bin
EOF
    [ "$status" -eq 0 ]
}

@test "Non-existent command" {
    run $test_shell <<EOF
not_a_command
EOF
    [ "$status" -ne 0 ]  # Expect failure
}

@test "Extra Credit: rc command" {
    run $test_shell <<EOF
not_a_command
rc
EOF
    [ "$output" = "2" ]  # Should print last return code
}

