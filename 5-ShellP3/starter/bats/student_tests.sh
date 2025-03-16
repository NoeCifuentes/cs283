#!/usr/bin/env bats

@test "Check ls runs successfully" {
    run ./dsh <<EOF
ls
EOF
    [ "$status" -eq 0 ]
}

@test "Test simple pipe" {
    run ./dsh <<EOF
ls | grep .c
EOF
    [[ "$output" == *".c"* ]]
    [ "$status" -eq 0 ]
}

@test "Test multiple pipes" {
    run ./dsh <<EOF
ls | grep .c | wc -l
EOF
    [[ "$output" =~ [0-9] ]]
    [ "$status" -eq 0 ]
}

@test "Test cd command" {
    run ./dsh <<EOF
cd /
pwd
EOF
    [[ "$output" == *"/"* ]]
    [ "$status" -eq 0 ]
}

@test "Test output redirection" {
    run ./dsh <<EOF
echo "hello world" > testfile.txt
cat testfile.txt
EOF
    [[ "$output" == *"hello world"* ]]
    [ "$status" -eq 0 ]
    rm -f testfile.txt
}

@test "Test input redirection" {
    echo "test input" > input.txt
    run ./dsh <<EOF
cat < input.txt
EOF
    [[ "$output" == *"test input"* ]]
    [ "$status" -eq 0 ]
    rm -f input.txt
}

