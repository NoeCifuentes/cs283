## Assignment 2 Questions

#### Directions
Please answer the following questions and submit in your repo for the second assignment.  Please keep the answers as short and concise as possible.

1. In this assignment I asked you provide an implementation for the `get_student(...)` function because I think it improves the overall design of the database application.   After you implemented your solution do you agree that externalizing `get_student(...)` into its own function is a good design strategy?  Briefly describe why or why not.

    > **Answer:** Yes, externalizing `get_student(...)` into its own function is a good design strategy since it encapsulates the retrieval logic, which gives reusability and reduces redundancy. The approach simplifies maintenance and debugging, as the function can be independently tested and updated.

2. Another interesting aspect of the `get_student(...)` function is how its function prototype requires the caller to provide the storage for the `student_t` structure:

    ```c
    int get_student(int fd, int id, student_t *s);
    ```

    Notice that the last parameter is a pointer to storage **provided by the caller** to be used by this function to populate information about the desired student that is queried from the database file. This is a common convention (called pass-by-reference) in the `C` programming language. 

    In other programming languages an approach like the one shown below would be more idiomatic for creating a function like `get_student()` (specifically the storage is provided by the `get_student(...)` function itself):

    ```c
    //Lookup student from the database
    // IF FOUND: return pointer to student data
    // IF NOT FOUND: return NULL
    student_t *get_student(int fd, int id){
        student_t student;
        bool student_found = false;
        
        //code that looks for the student and if
        //found populates the student structure
        //The found_student variable will be set
        //to true if the student is in the database
        //or false otherwise.

        if (student_found)
            return &student;
        else
            return NULL;
    }
    ```
    Can you think of any reason why the above implementation would be a **very bad idea** using the C programming language?  Specifically, address why the above code introduces a subtle bug that could be hard to identify at runtime? 

    > **Answer:** Returning a pointer to a local variable leads to undefined behavior because local variables are stored on the stack and their memory is reclaimed when the function exits. This results in a dangling pointer that references invalid memory, which can cause subtle and hard-to-diagnose bugs during runtime.

3. Another way the `get_student(...)` function could be implemented is as follows:

    ```c
    //Lookup student from the database
    // IF FOUND: return pointer to student data
    // IF NOT FOUND or memory allocation error: return NULL
    student_t *get_student(int fd, int id){
        student_t *pstudent;
        bool student_found = false;

        pstudent = malloc(sizeof(student_t));
        if (pstudent == NULL)
            return NULL;
        
        //code that looks for the student and if
        //found populates the student structure
        //The found_student variable will be set
        //to true if the student is in the database
        //or false otherwise.

        if (student_found){
            return pstudent;
        }
        else {
            free(pstudent);
            return NULL;
        }
    }
    ```
    In this implementation the storage for the student record is allocated on the heap using `malloc()` and passed back to the caller when the function returns. What do you think about this alternative implementation of `get_student(...)`?  Address in your answer why it would work, but also think about any potential problems it could cause.  
    
    > **Answer:** Dynamically allocating memory allows the function to safely return a pointer to the caller, but it introduces the risk of memory leaks if the caller forgets to free the allocated memory. also `malloc()` can fail returning `NULL` which needs error handling. Mismanagement of heap memory will lead to crashes.

4. Let's take a look at how storage is managed for our simple database. Recall that all student records are stored on disk using the layout of the `student_t` structure (which has a size of 64 bytes). Let's start with a fresh database by deleting the `student.db` file using the command `rm ./student.db`. Now that we have an empty database, let's add a few students and see what is happening under the covers. Consider the following sequence of commands:

    ```bash
    > ./sdbsc -a 1 john doe 345
    > ls -l ./student.db
        -rw-r----- 1 bsm23 bsm23 128 Jan 17 10:01 ./student.db
    > du -h ./student.db
        4.0K    ./student.db
    > ./sdbsc -a 3 jane doe 390
    > ls -l ./student.db
        -rw-r----- 1 bsm23 bsm23 256 Jan 17 10:02 ./student.db
    > du -h ./student.db
        4.0K    ./student.db
    > ./sdbsc -a 63 jim doe 285 
    > du -h ./student.db
        4.0K    ./student.db
    > ./sdbsc -a 64 janet doe 310
    > du -h ./student.db
        8.0K    ./student.db
    > ls -l ./student.db
        -rw-r----- 1 bsm23 bsm23 4160 Jan 17 10:03 ./student.db
    ```

    - **Why was the file size reported by the `ls` command 128 bytes after adding student with ID=1, 256 after adding student with ID=3, and 4160 after adding the student with ID=64?**

        > **Answer:** The `ls` command shows the logical file size, which includes all data and empty areas between records. Sparse files optimize storage by not allocating physical space for these gaps, leading to larger logical sizes.

    - **Why did the total storage used on the disk remain unchanged when we added the student with ID=1, ID=3, and ID=63, but increased from 4K to 8K when we added the student with ID=64?** 

        > **Answer:** Sparse files only allocate physical storage for actual data, leaving gaps unallocated. The disk usage remained 4K for earlier records, but adding ID=64 filled a large gap, requiring allocation of new blocks, which increased disk usage to 8K.

    - **Why did adding a student with a large ID (99999) drastically increase the file size but not the disk usage?**

        ```bash
        > ./sdbsc -a 99999 big dude 205 
        > ls -l ./student.db
        -rw-r----- 1 bsm23 bsm23 6400000 Jan 17 10:28 ./student.db
        > du -h ./student.db
        12K     ./student.db
        ```

        > **Answer:** The file size grew to 6.4 MB because of the large offset needed for the student with ID 99999, but sparse files do not allocate physical storage for the empty regions, keeping the actual disk usage minimal at 12K.

