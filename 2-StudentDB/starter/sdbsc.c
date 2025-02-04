#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>

#include "db.h"
#include "sdbsc.h"

// Helper function to check if a record is empty (all zero bytes)
bool is_empty_record(const student_t *student) {
    return memcmp(student, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) == 0;
}

/*
 * open_db - Opens the database file and creates it if needed.
 */
int open_db(char *dbFile, bool should_truncate) {
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;  // rw-rw----
    int flags = O_RDWR | O_CREAT;
    if (should_truncate) flags |= O_TRUNC;

    int fd = open(dbFile, flags, mode);
    if (fd == -1) {
        printf(M_ERR_DB_OPEN);
        return ERR_DB_FILE;
    }
    return fd;
}

/*
 * get_student - Fetches a student record by ID.
 */
int get_student(int fd, int id, student_t *s) {
    off_t offset = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        printf(M_ERR_DB_READ);
        return ERR_DB_FILE;
    }

    if (read(fd, s, STUDENT_RECORD_SIZE) != STUDENT_RECORD_SIZE) {
        return SRCH_NOT_FOUND;
    }

    if (s->id == DELETED_STUDENT_ID) {
        return SRCH_NOT_FOUND;
    }

    return NO_ERROR;
}

/*
 * add_student - Adds a student record if the slot is empty.
 */
int add_student(int fd, int id, char *fname, char *lname, int gpa) {
    student_t student;
    if (get_student(fd, id, &student) == NO_ERROR) {
        printf(M_ERR_DB_ADD_DUP, id);
        return ERR_DB_OP;
    }

    student_t new_student = {id, "", "", gpa};
    strncpy(new_student.fname, fname, sizeof(new_student.fname) - 1);
    strncpy(new_student.lname, lname, sizeof(new_student.lname) - 1);

    off_t offset = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offset, SEEK_SET) == -1 || write(fd, &new_student, STUDENT_RECORD_SIZE) != STUDENT_RECORD_SIZE) {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }

    printf(M_STD_ADDED, id);
    return NO_ERROR;
}

/*
 * del_student - Deletes a student record by zeroing it out.
 */
int del_student(int fd, int id) {
    student_t student;
    if (get_student(fd, id, &student) != NO_ERROR) {
        printf(M_STD_NOT_FND_MSG, id);
        return ERR_DB_OP;
    }

    off_t offset = id * STUDENT_RECORD_SIZE;
    if (lseek(fd, offset, SEEK_SET) == -1 || write(fd, &EMPTY_STUDENT_RECORD, STUDENT_RECORD_SIZE) != STUDENT_RECORD_SIZE) {
        printf(M_ERR_DB_WRITE);
        return ERR_DB_FILE;
    }

    printf(M_STD_DEL_MSG, id);
    return NO_ERROR;
}

/*
 * count_db_records - Counts the number of active student records.
 */
int count_db_records(int fd) {
    student_t student;
    int count = 0;

    lseek(fd, 0, SEEK_SET);
    while (read(fd, &student, STUDENT_RECORD_SIZE) == STUDENT_RECORD_SIZE) {
        if (!is_empty_record(&student)) count++;
    }

    if (count == 0) {
        printf(M_DB_EMPTY);
    } else {
        printf(M_DB_RECORD_CNT, count);
    }

    return count;
}

/*
 * print_db - Prints all active student records.
 */
int print_db(int fd) {
    student_t student;
    bool header_printed = false;

    lseek(fd, 0, SEEK_SET);
    while (read(fd, &student, STUDENT_RECORD_SIZE) == STUDENT_RECORD_SIZE) {
        if (!is_empty_record(&student)) {
            if (!header_printed) {
                // Correct header format
                printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST_NAME", "LAST_NAME", "GPA");
                header_printed = true;
            }
            printf(STUDENT_PRINT_FMT_STRING, student.id, student.fname, student.lname, student.gpa / 100.0);
        }
    }

    if (!header_printed) {
        printf(M_DB_EMPTY);
    }

    return NO_ERROR;
}

/*
 * print_student - Prints a single student record.
 */
void print_student(student_t *s) {
    if (!s || s->id == DELETED_STUDENT_ID) {
        printf(M_ERR_STD_PRINT);
        return;
    }

    // Correct header format
    printf(STUDENT_PRINT_HDR_STRING, "ID", "FIRST_NAME", "LAST_NAME", "GPA");
    printf(STUDENT_PRINT_FMT_STRING, s->id, s->fname, s->lname, s->gpa / 100.0);
}

/*
 * compress_db - Compresses the database by removing zeroed records.
 */
int compress_db(int fd) {
    int tmp_fd = open(TMP_DB_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0660);
    if (tmp_fd == -1) {
        printf(M_ERR_DB_CREATE);
        return ERR_DB_FILE;
    }

    student_t student;
    while (read(fd, &student, STUDENT_RECORD_SIZE) == STUDENT_RECORD_SIZE) {
        if (!is_empty_record(&student)) {
            if (write(tmp_fd, &student, STUDENT_RECORD_SIZE) != STUDENT_RECORD_SIZE) {
                printf(M_ERR_DB_WRITE);
                close(tmp_fd);
                return ERR_DB_FILE;
            }
        }
    }

    close(fd);

    if (rename(TMP_DB_FILE, DB_FILE) == -1) {
        printf(M_ERR_DB_CREATE);
        return ERR_DB_FILE;
    }

    tmp_fd = open(DB_FILE, O_RDWR);
    if (tmp_fd == -1) {
        printf(M_ERR_DB_OPEN);
        return ERR_DB_FILE;
    }

    printf(M_DB_COMPRESSED_OK);
    return tmp_fd;
}

/*
 * validate_range - Validates that ID and GPA are within allowable ranges.
 */
int validate_range(int id, int gpa) {
    if (id < MIN_STD_ID || id > MAX_STD_ID || gpa < MIN_STD_GPA || gpa > MAX_STD_GPA) {
        return EXIT_FAIL_ARGS;
    }
    return NO_ERROR;
}

/*
 * usage - Prints the program's usage information.
 */
void usage(char *exename) {
    printf("usage: %s -[h|a|c|d|f|p|z] options. Where:\n", exename);
    printf("\t-h:  prints help\n");
    printf("\t-a id first_name last_name gpa(as 3 digit int): adds a student\n");
    printf("\t-c:  counts the records in the database\n");
    printf("\t-d id: deletes a student\n");
    printf("\t-f id: finds and prints a student in the database\n");
    printf("\t-p:  prints all records in the student database\n");
    printf("\t-x:  compresses the database file (extra credit)\n");
    printf("\t-z:  zero db file (remove all records)\n");
}

/*
 * main - Entry point of the program.
 */
int main(int argc, char *argv[]) {
    char opt;
    int fd, rc, exit_code = EXIT_OK, id, gpa;
    student_t student = {0};

    if (argc < 2 || argv[1][0] != '-') {
        usage(argv[0]);
        exit(EXIT_FAIL_ARGS);
    }

    opt = argv[1][1];

    if (opt == 'h') {
        usage(argv[0]);
        exit(EXIT_OK);
    }

    fd = open_db(DB_FILE, false);
    if (fd < 0) exit(EXIT_FAIL_DB);

    switch (opt) {
        case 'a':
            if (argc != 6) {
                usage(argv[0]);
                exit_code = EXIT_FAIL_ARGS;
                break;
            }
            id = atoi(argv[2]);
            gpa = atoi(argv[5]);
            exit_code = validate_range(id, gpa);
            if (exit_code == EXIT_FAIL_ARGS) {
                printf(M_ERR_STD_RNG);
                break;
            }
            rc = add_student(fd, id, argv[3], argv[4], gpa);
            if (rc < 0) exit_code = EXIT_FAIL_DB;
            break;

        case 'c':
            rc = count_db_records(fd);
            if (rc < 0) exit_code = EXIT_FAIL_DB;
            break;

        case 'd':
            if (argc != 3) {
                usage(argv[0]);
                exit_code = EXIT_FAIL_ARGS;
                break;
            }
            id = atoi(argv[2]);
            rc = del_student(fd, id);
            if (rc < 0) exit_code = EXIT_FAIL_DB;
            break;

        case 'f':
            if (argc != 3) {
                usage(argv[0]);
                exit_code = EXIT_FAIL_ARGS;
                break;
            }
            id = atoi(argv[2]);
            rc = get_student(fd, id, &student);
            if (rc == NO_ERROR) {
                print_student(&student);
            } else if (rc == SRCH_NOT_FOUND) {
                printf(M_STD_NOT_FND_MSG, id);
                exit_code = EXIT_FAIL_DB;
            } else {
                printf(M_ERR_DB_READ);
                exit_code = EXIT_FAIL_DB;
            }
            break;

        case 'p':
            rc = print_db(fd);
            if (rc < 0) exit_code = EXIT_FAIL_DB;
            break;

        case 'x':
            fd = compress_db(fd);
            if (fd < 0) exit_code = EXIT_FAIL_DB;
            break;

        case 'z':
            close(fd);
            fd = open_db(DB_FILE, true);
            if (fd < 0) {
                exit_code = EXIT_FAIL_DB;
                break;
            }
            printf(M_DB_ZERO_OK);
            break;

        default:
            usage(argv[0]);
            exit_code = EXIT_FAIL_ARGS;
    }

    close(fd);
    exit(exit_code);
}

