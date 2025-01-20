#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#define BUFFER_SZ 50

void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);
int count_words(char *, int, int);
int reverse_string(char *, int, int);
int print_words(char *, int, int);
int replace_string(char *, int, int, char *, char *);

void usage(char *exename) {
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);
}

void print_buff(char *buff, int len) {
    printf("Buffer:  [");
    for (int i = 0; i < len; i++) {
        putchar(*(buff + i));
    }
    printf("]\n");
}

int setup_buff(char *buff, char *user_str, int len) {
    if (!buff || !user_str || len <= 0) return -2;
    
    int in_idx = 0;
    int out_idx = 0;
    int prev_space = 1;
    
    while (*(user_str + in_idx) != '\0') {
        char c = *(user_str + in_idx++);
        
        if (c == ' ' || c == '\t') {
            if (!prev_space && out_idx < len) {
                *(buff + out_idx++) = ' ';
                prev_space = 1;
            }
        } else {
            if (out_idx >= len) return -1;
            *(buff + out_idx++) = c;
            prev_space = 0;
        }
    }
    
    if (out_idx > 0 && *(buff + out_idx - 1) == ' ') {
        out_idx--;
    }
    
    int str_len = out_idx;
    
    while (out_idx < len) {
        *(buff + out_idx++) = '.';
    }
    
    return str_len;
}

int count_words(char *buff, int len, int str_len) {
    if (!buff || len <= 0 || str_len <= 0 || str_len > len) return -1;
    
    int count = 0;
    int in_word = 0;
    char *ptr = buff;
    char *end = buff + str_len;
    
    while (ptr < end) {
        if (*ptr == ' ') {
            in_word = 0;
        } else if (!in_word) {
            count++;
            in_word = 1;
        }
        ptr++;
    }
    
    return count;
}

int reverse_string(char *buff, int len, int str_len) {
    if (!buff || len <= 0 || str_len <= 0 || str_len > len) return -1;
    
    char *start = buff;
    char *end = buff + str_len - 1;
    char temp;
    
    while (start < end) {
        temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
    
    return 0;
}

int print_words(char *buff, int len, int str_len) {
    if (!buff || len <= 0 || str_len <= 0 || str_len > len) return -1;
    
    printf("Word Print\n----------\n");
    int word_count = 0;
    char *ptr = buff;
    char *word_start = ptr;
    int word_len = 0;
    int in_word = 0;
    
    while (ptr < buff + str_len) {
        if (*ptr == ' ') {
            if (in_word) {
                word_count++;
                printf("%d. ", word_count);
                char *temp = word_start;
                while (temp < ptr) {
                    putchar(*temp++);
                }
                printf("(%d)\n", word_len);
                word_len = 0;
                in_word = 0;
            }
        } else {
            if (!in_word) {
                word_start = ptr;
                in_word = 1;
            }
            word_len++;
        }
        ptr++;
    }
    
    if (in_word) {
        word_count++;
        printf("%d. ", word_count);
        char *temp = word_start;
        while (temp < ptr) {
            putchar(*temp++);
        }
        printf("(%d)\n", word_len);
    }
    
    printf("\nNumber of words returned: %d\n", word_count);
    return word_count;
}

int replace_string(char *buff, int len, int str_len, char *find, char *replace) {
    if (!buff || !find || !replace || len <= 0 || str_len <= 0) return -1;
    
    int find_len = 0;
    int replace_len = 0;
    char *ptr;
    
    for (ptr = find; *ptr != '\0'; ptr++) find_len++;
    for (ptr = replace; *ptr != '\0'; ptr++) replace_len++;
    
    int found = 0;
    char *start = buff;
    char *end = buff + str_len;
    char *match_start = NULL;
    
    while (start < end) {
        int matched = 1;
        char *cur_find = find;
        char *cur_buff = start;
        
        while (*cur_find != '\0' && cur_buff < end) {
            if (*cur_find != *cur_buff) {
                matched = 0;
                break;
            }
            cur_find++;
            cur_buff++;
        }
        
        if (matched && *cur_find == '\0' && 
            (cur_buff == end || *cur_buff == ' ')) {
            found = 1;
            match_start = start;
            break;
        }
        
        while (start < end && *start != ' ') start++;
        while (start < end && *start == ' ') start++;
    }
    
    if (!found) return -1;
    
    int new_len = str_len - find_len + replace_len;
    if (new_len > len) new_len = len;
    
    if (replace_len != find_len) {
        int shift = replace_len - find_len;
        if (shift > 0) {
            for (int i = str_len - 1; i >= match_start - buff + find_len; i--) {
                if (i + shift < len) {
                    *(buff + i + shift) = *(buff + i);
                }
            }
        } else {
            for (int i = match_start - buff + find_len; i < str_len; i++) {
                *(buff + i + shift) = *(buff + i);
            }
        }
    }
    
    ptr = replace;
    char *dst = match_start;
    while (*ptr != '\0' && dst < buff + len) {
        *dst++ = *ptr++;
    }
    
    char *dot_start = buff + new_len;
    while (dot_start < buff + len) {
        *dot_start++ = '.';
    }
    
    return new_len;
}

int main(int argc, char *argv[]) {
    char *buff;
    char *input_string;
    char opt;
    int rc;
    int user_str_len;

    if ((argc < 2) || (*argv[1] != '-')) {
        usage(argv[0]);
        exit(1);
    }

    opt = *(argv[1] + 1);

    if (opt == 'h') {
        usage(argv[0]);
        exit(0);
    }

    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2];

    buff = (char *)malloc(BUFFER_SZ);
    if (!buff) {
        printf("Error: Failed to allocate memory\n");
        exit(99);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);
    if (user_str_len < 0) {
        printf("Error setting up buffer, error = %d\n", user_str_len);
        free(buff);
        exit(2);
    }

    switch (opt) {
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);
            if (rc < 0) {
                printf("Error counting words, rc = %d\n", rc);
                free(buff);
                exit(2);
            }
            printf("Word Count: %d\n", rc);
            break;
            
        case 'r':
            rc = reverse_string(buff, BUFFER_SZ, user_str_len);
            if (rc < 0) {
                printf("Error reversing string, rc = %d\n", rc);
                free(buff);
                exit(2);
            }
            break;
            
        case 'w':
            rc = print_words(buff, BUFFER_SZ, user_str_len);
            if (rc < 0) {
                printf("Error printing words, rc = %d\n", rc);
                free(buff);
                exit(2);
            }
            break;
            
        case 'x':
            if (argc < 5) {
                printf("Error: -x requires two additional arguments\n");
                free(buff);
                exit(1);
            }
            rc = replace_string(buff, BUFFER_SZ, user_str_len, argv[3], argv[4]);
            if (rc < 0) {
                printf("Error: String '%s' not found in text\n", argv[3]);
                free(buff);
                exit(3);
            }
            user_str_len = rc;
            break;
            
        default:
            usage(argv[0]);
            free(buff);
            exit(1);
    }

    print_buff(buff, BUFFER_SZ);
    free(buff);
    exit(0);
}

