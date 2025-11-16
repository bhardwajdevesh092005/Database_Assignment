#include "sp.h"
#include "string.h"
#include "stdio.h"
#include "pf.h"
#include "am.h"
#include "bulklayer/bl.h"
#include <time.h>

#define EQ 1
#define LT 2
#define GT 3
#define LE 4
#define GE 5
#define NE 6

#define MAX_SLOTS_PER_PAGE 4096
int recIds[200];
int roll_nos[200];
int data_file_fd;
int index_fd_1;
typedef struct Student 
{
    int roll_no;
    char data[80];
} Student;

int serialise_st(Student *st, char* buff){
    sprintf(buff,"%d %s",st->roll_no,st->data);
    return strlen(buff)+1;
}

int getRecId(SP_RID sp){
    return MAX_SLOTS_PER_PAGE*sp.page_num + sp.slot_num;
}

int insert_data(char* text_file){
    char line[200];
    
    /* Remove old file if it exists */
    SP_DestroyFile("student_db");
    
    data_file_fd = SP_CreateFile("student_db");
    if(data_file_fd<0){
        printf("Error creating student_db file\n");
        return -1;
    }
    data_file_fd = SP_OpenFile("student_db");
    if(data_file_fd<0){
        printf("Error opening file\n");
        return -1;
    }
    FILE* text_file_p = fopen(text_file,"r");
    if(text_file_p==NULL){
        printf("Error opening text file\n");
        return -1;
    }
    int i = 0;
    /* Read all records from text file until end of file */
    while(fgets(line,200,text_file_p)!=NULL){
        Student st;       
        sscanf(line,"%d %100s",&st.roll_no,st.data);
        char buffer[200];
        int rec_size = serialise_st(&st,buffer);
        SP_RID rid;
        int err = SP_InsertRec(data_file_fd,buffer,rec_size,&rid);
        if(err<0){
            printf("Error inserting record %d\n",i);
            fclose(text_file_p);
            return -1;
        }   
        err = SP_GetThisRec(data_file_fd,rid,buffer,200);
        if(err<0){
            printf("Error fetching record just inserted %d\n",i);
            fclose(text_file_p);
            return -1;
        }
        if(i<200){
            recIds[i] = getRecId(rid);
            roll_nos[i] = st.roll_no;
        }
        i++;
    }
    /* nm_records = i; Store total number of records */
    printf("Inserted %d records into data file\n", i);
    fclose(text_file_p);
    SP_CloseFile(data_file_fd);
    return 0;
}

/* Method 1: Build index from existing data file (unsorted bulk load) */
int create_index_from_existing_file(char* index_file){
    printf("\n=== METHOD 1: Bulk Loading from Existing File (Unsorted) ===\n");
    
    /* Remove old index file if it exists */
    PF_DestroyFile("student_db_bulk.0");
    
    AM_CreateIndex("student_db_bulk",0,'i',4);
    
    /* Open the data file fresh */
    int data_fd = SP_OpenFile("student_db");
    if (data_fd < 0) {
        printf("Error opening data file for indexing\n");
        return -1;
    }
    
    int index_fd = PF_OpenFile("student_db_bulk.0",0);
    if(index_fd<0){
        printf("Error opening index file\n");
        SP_CloseFile(data_fd);
        return -1;
    }

    char buffer[200];
    Student st;
    SP_RID rid;
    int err;
    int record_count = 0;

    /* Initialize stats for index construction */
    PF_InitStats();
    clock_t start_time = clock();

    printf("Starting bulk index creation from data file using linear scan...\n");

    err = SP_GetFirstRec(data_fd, buffer, sizeof(buffer), &rid);
    if (err != SP_OK && err != SP_EOF) {
        printf("Error: SP_GetFirstRec failed with error code %d\n", err);
        printf("This might indicate the data file is empty or has no valid pages.\n");
        SP_CloseFile(data_fd);
        PF_CloseFile(index_fd);
        return -1;
    }
    
    while (err == SP_OK) {
        sscanf(buffer, "%d %100s", &st.roll_no, st.data);
        
        int recId = getRecId(rid);

        int index_err = AM_InsertEntry(index_fd, 'i', 4, (char*)(&st.roll_no), recId);
        if (index_err < 0) {
            printf("Error inserting index entry for roll no %d\n", st.roll_no);
        } else {
            record_count++;
        }

        SP_RID next_rid;
        err = SP_GetNextRec(data_fd, &rid, buffer, sizeof(buffer), &next_rid);
        if (err == SP_OK) {
            rid = next_rid;
        }
    }

    clock_t end_time = clock();
    double build_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    if (err != SP_EOF && err != SP_OK) {
        printf("Warning: Scan ended with code %d (expected SP_EOF). Processed %d records.\n", err, record_count);
    }

    printf("Index created successfully with %d entries\n", record_count);
    printf("Index construction time: %f seconds\n", build_time);
    printf("Page access statistics during index construction:\n");
    PF_PrintStats();

    SP_CloseFile(data_fd);
    PF_CloseFile(index_fd);
    return index_fd;
}

/*
 * Method 2: Build data file and index incrementally.
 * This simulates starting with an empty file and adding records one by one.
 */
int build_index_incrementally(char* text_file) {
    printf("\n=== METHOD 2: Incremental Index Construction ===\n");

    /* Remove old files if they exist */
    SP_DestroyFile("student_db_inc");
    PF_DestroyFile("student_index_inc.0");
    
    if (SP_CreateFile("student_db_inc") < 0) {
        printf("Error creating incremental student DB file\n");
        return -1;
    }
    if (AM_CreateIndex("student_index_inc", 0, 'i', 4) < 0) {
        printf("Error creating incremental index file\n");
        return -1;
    }
    int data_fd = SP_OpenFile("student_db_inc");
    if (data_fd < 0) {
        printf("Error opening incremental student DB file\n");
        return -1;
    }
    int index_fd = PF_OpenFile("student_index_inc.0",0);
    if (index_fd < 0) {
        printf("Error opening incremental index file\n");
        SP_CloseFile(data_fd);
        return -1;
    }

    FILE* text_file_p = fopen(text_file, "r");
    if (text_file_p == NULL) {
        printf("Error opening text file\n");
        SP_CloseFile(data_fd);
        PF_CloseFile(index_fd);
        return -1;
    }

    /* Initialize stats for index construction */
    PF_InitStats();
    clock_t start_time = clock();

    char line[200];
    int i = 0;
    /* Read all records from text file until end of file */
    while (fgets(line, 200, text_file_p) != NULL) {
        Student st;
        sscanf(line, "%d %100s", &st.roll_no, st.data);

        char buffer[200];
        int rec_size = serialise_st(&st, buffer);
        SP_RID rid;
        if (SP_InsertRec(data_fd, buffer, rec_size, &rid) < 0) {
            printf("Error inserting record %d into data file\n", i);
            break; 
        }

        int recId = getRecId(rid);
        if (AM_InsertEntry(index_fd, 'i', 4, (char*)(&st.roll_no), recId) < 0) {
            printf("Error inserting index entry for record %d\n", i);
            break;
        }
        i++;
    }

    clock_t end_time = clock();
    double build_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("Incremental index construction complete with %d entries\n", i);
    printf("Index construction time: %f seconds\n", build_time);
    printf("Page access statistics during index construction:\n");
    PF_PrintStats();

    fclose(text_file_p);
    SP_CloseFile(data_fd);
    PF_CloseFile(index_fd);

    return index_fd; 
}


/* Helper function for sorting students by roll_no */
int compare_students(const void *a, const void *b) {
    Student *st1 = (Student *)a;
    Student *st2 = (Student *)b;
    return st1->roll_no - st2->roll_no;
}

/*
 * Method 3: Efficient bulk-loading from pre-sorted data
 * This method first sorts the data and then builds the index,
 * which can be much more efficient than inserting in random order.
 */
int build_index_sorted_bulk(char* text_file) {
    printf("\n=== METHOD 3: Efficient Bulk-Loading from Pre-Sorted Data ===\n");
    
    Student students[200];
    int num_students = 0;
    
    /* Step 1: Read all records from text file */
    FILE* text_file_p = fopen(text_file, "r");
    if (text_file_p == NULL) {
        printf("Error opening text file\n");
        return -1;
    }
    
    char line[200];
    while (fgets(line, 200, text_file_p) != NULL && num_students < 200) {
        sscanf(line, "%d %100s", &students[num_students].roll_no, students[num_students].data);
        num_students++;
    }
    fclose(text_file_p);
    
    printf("Loaded %d student records\n", num_students);
    
    /* Step 2: Sort students by roll_no */
    printf("Sorting students by roll_no...\n");
    qsort(students, num_students, sizeof(Student), compare_students);
    printf("Sorting complete\n");
    
    /* Remove old files if they exist */
    SP_DestroyFile("student_db_sorted");
    PF_DestroyFile("student_index_sorted.0");
    
    /* Step 3: Create data file and index with sorted data */
    if (SP_CreateFile("student_db_sorted") < 0) {
        printf("Error creating sorted student DB file\n");
        return -1;
    }
    if (AM_CreateIndex("student_index_sorted", 0, 'i', 4) < 0) {
        printf("Error creating sorted index file\n");
        return -1;
    }
    
    int data_fd = SP_OpenFile("student_db_sorted");
    if (data_fd < 0) {
        printf("Error opening sorted student DB file\n");
        return -1;
    }
    
    int index_fd = PF_OpenFile("student_index_sorted.0", 0);
    if (index_fd < 0) {
        printf("Error opening sorted index file\n");
        SP_CloseFile(data_fd);
        return -1;
    }
    
    /* Initialize stats for index construction */
    PF_InitStats();
    clock_t start_time = clock();
    
    /* Step 4: Insert sorted records into data file and index */
    int i;
    for (i = 0; i < num_students; i++) {
        char buffer[200];
        int rec_size = serialise_st(&students[i], buffer);
        SP_RID rid;
        
        if (SP_InsertRec(data_fd, buffer, rec_size, &rid) < 0) {
            printf("Error inserting sorted record %d into data file\n", i);
            break;
        }
        
        int recId = getRecId(rid);
        if (AM_InsertEntry(index_fd, 'i', 4, (char*)(&students[i].roll_no), recId) < 0) {
            printf("Error inserting sorted index entry for record %d\n", i);
            break;
        }
    }
    
    clock_t end_time = clock();
    double build_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("Sorted bulk index construction complete with %d entries\n", i);
    printf("Index construction time: %f seconds\n", build_time);
    printf("Page access statistics during index construction:\n");
    PF_PrintStats();
    
    SP_CloseFile(data_fd);
    PF_CloseFile(index_fd);
    
    return index_fd;
}

/*
 * Method 3B: True Bottom-Up Bulk-Loading using BL layer
 * This implements the optimal O(N) bulk-loading algorithm
 */
int build_index_true_bulkload(char* text_file, int num_records) {
    printf("\n=== METHOD 3B: True Bottom-Up Bulk-Loading (O(N) Algorithm) ===\n");
    
    BL_KeyVal *sorted_data;
    FILE* text_file_p;
    char line[200];
    int i, count;
    int indexFd;
    clock_t start_time, end_time;
    double build_time;
    Student st;
    
    /* Allocate array for sorted data */
    sorted_data = (BL_KeyVal*)malloc(num_records * sizeof(BL_KeyVal));
    if (!sorted_data) {
        printf("Error: Failed to allocate memory for sorted data\n");
        return -1;
    }
    
    /* Read all records from text file */
    text_file_p = fopen(text_file, "r");
    if (text_file_p == NULL) {
        printf("Error opening text file\n");
        free(sorted_data);
        return -1;
    }
    
    printf("Loading %d records from file...\n", num_records);
    count = 0;
    while (fgets(line, 200, text_file_p) != NULL && count < num_records) {
        sscanf(line, "%d %100s", &st.roll_no, st.data);
        sorted_data[count].key = st.roll_no;
        sorted_data[count].value = count; /* Store record index as value */
        count++;
    }
    fclose(text_file_p);
    
    printf("Loaded %d student records\n", count);
    
    /* Sort the data by key (roll_no) - using bubble sort for simplicity */
    printf("Sorting students by roll_no...\n");
    for (i = 0; i < count - 1; i++) {
        int j;
        for (j = 0; j < count - i - 1; j++) {
            if (sorted_data[j].key > sorted_data[j + 1].key) {
                BL_KeyVal temp = sorted_data[j];
                sorted_data[j] = sorted_data[j + 1];
                sorted_data[j + 1] = temp;
            }
        }
    }
    printf("Sorting complete\n");
    
    /* Clean up old index file */
    BL_DestroyIndex("student_index_bulkload.db");
    
    /* Create new index */
    if (BL_CreateIndex("student_index_bulkload.db") != BL_OK) {
        printf("Error creating BL index file\n");
        free(sorted_data);
        return -1;
    }
    
    /* Open the index */
    indexFd = BL_OpenIndex("student_index_bulkload.db");
    if (indexFd < 0) {
        printf("Error opening BL index file\n");
        free(sorted_data);
        return -1;
    }
    
    /* Perform bottom-up bulk-load */
    printf("Performing true bottom-up bulk-load (O(N) construction)...\n");
    PF_InitStats();
    start_time = clock();
    
    if (BL_BulkLoad(indexFd, sorted_data, count) != BL_OK) {
        printf("Error during bulk-load\n");
        BL_CloseIndex(indexFd);
        free(sorted_data);
        return -1;
    }
    
    end_time = clock();
    build_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("True bulk-load completed with %d entries\n", count);
    printf("Index construction time: %f seconds\n", build_time);
    printf("Page access statistics during index construction:\n");
    PF_PrintStats();
    
    /* Print BL index statistics */
    BL_PrintStats(indexFd);
    
    free(sorted_data);
    return indexFd;
}

/* Function to run queries on BL index */
void run_queries_on_bl_index(int indexFd, int num_queries) {
    int i, key, value;
    int not_found = 0;
    clock_t start_time, end_time;
    double query_time;
    
    printf("\n--- Running Queries on BL Index ---\n");
    printf("Executing %d random queries...\n", num_queries);
    
    PF_InitStats();
    start_time = clock();
    
    srand(time(NULL) + 12345); /* Different seed for variety */
    for (i = 0; i < num_queries; i++) {
        int random_index = rand() % 200;
        if (random_index < 200) {
            key = roll_nos[random_index];
            if (BL_Search(indexFd, key, &value) != BL_OK) {
                not_found++;
            }
        }
    }
    
    end_time = clock();
    query_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("\n--- Query Performance Report ---\n");
    printf("Total queries executed: %d\n", num_queries);
    printf("Records not found: %d\n", not_found);
    printf("Total query completion time: %f seconds\n", query_time);
    
    printf("\n--- Page Access Statistics ---\n");
    PF_PrintStats();
}


/* Function to run queries on the created index and measure performance */
void run_queries_on_index(const char* index_filename, int num_records_to_query) {
    printf("\n--- Running Queries on %s ---\n", index_filename);

    if (num_records_to_query <= 0 || num_records_to_query > 200) {
        printf("Invalid number of records to query. Defaulting to 10.\n");
        num_records_to_query = 10;
    }
    int query_keys[num_records_to_query];
    srand(time(NULL));
    printf("Selecting %d random roll numbers to query...\n", num_records_to_query);
    int i;
    for (i = 0; i < num_records_to_query; i++) {
        int random_index = rand() % 200;
        query_keys[i] = roll_nos[random_index];
    }

    PF_InitStats();

    clock_t start_time = clock();
    int index_fd = PF_OpenFile(index_filename, 0);
    if (index_fd < 0) {
        printf("Error opening index file %s for querying.\n", index_filename);
        return;
    }
    int not_found = 0;
    for (i = 0; i < num_records_to_query; i++) {
        int key = query_keys[i];
        int scan_fd = AM_OpenIndexScan(index_fd, 'i', 4, EQ, (char*)&key);
        if (scan_fd >= 0) {
            int rid = AM_FindNextEntry(scan_fd);
            if (rid < 0) {
                not_found++;
            }
            AM_CloseIndexScan(scan_fd);
        } else {
            AM_PrintError("Error opening index scan");
            not_found++;
        }
    }
    PF_CloseFile(index_fd);

    clock_t end_time = clock();
    double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;

    printf("\n--- Query Performance Report ---\n");
    printf("Total queries executed: %d\n", num_records_to_query);
    printf("Records not found: %d\n", not_found);
    printf("Total query completion time: %f seconds\n", time_taken);
    
    printf("\n--- Page Access Statistics ---\n");
    PF_PrintStats();
}

int main(){
    printf("========================================\n");
    printf("  INDEX CONSTRUCTION PERFORMANCE TEST\n");
    printf("========================================\n\n");
    
    /* Prepare data file for Method 1 */
    printf("--- Preparing Data File ---\n");
    int status = insert_data("student.txt");
    if(status<0){
        printf("Error inserting data\n");
        return -1;
    }
    printf("Data file created successfully\n\n");
    
    /* Method 1: Bulk Loading from Existing File (Unsorted) */
    index_fd_1 = create_index_from_existing_file("student_db_bulk");
    if (index_fd_1 >= 0) {
        run_queries_on_index("student_db_bulk.0", 100); 
    }

    /* Method 2: Incremental Indexing */
    int index_fd_2 = build_index_incrementally("student.txt");
    if (index_fd_2 >= 0) {
        run_queries_on_index("student_index_inc.0", 100);
    }
    
    /* Method 3A: Efficient Bulk-Loading from Pre-Sorted Data (AM_InsertEntry) */
    int index_fd_3 = build_index_sorted_bulk("student.txt");
    if (index_fd_3 >= 0) {
        run_queries_on_index("student_index_sorted.0", 100);
    }
    
    /* Method 3B: True Bottom-Up Bulk-Loading using BL layer */
    int bl_status = build_index_true_bulkload("student.txt", 17814);
    if (bl_status >= 0) {
        run_queries_on_bl_index(bl_status, 100);
    }
    
    printf("\n========================================\n");
    printf("  PERFORMANCE COMPARISON SUMMARY\n");
    printf("========================================\n");
    printf("All four indexing methods have been tested.\n");
    printf("Compare the index construction times and page access statistics above.\n");
    printf("Expected results:\n");
    printf("  - Method 3B (True O(N) Bulk-Load) should be the fastest for sorted data\n");
    printf("  - Method 3A (Sorted AM_InsertEntry) is O(N log N)\n");
    printf("  - Method 1 and 2 should have similar performance\n");
    printf("  - Query performance should be similar across all methods\n");
    printf("========================================\n");

    return 0;
}