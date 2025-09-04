#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Student Database Management System (C - Minimal, File-Based)
 * ------------------------------------------------------------
 * - Stores records in a plain text file "students.txt" (CSV-like format).
 * - Demonstrates Data Structures (struct), basic file I/O, and menu-driven UI.
 * - Operations: Add, List, Delete by ID.
 *
 * NOTE: This C version stays minimal on purpose (no SQLite/OOP/encryption)
 *       to keep it portable and easy to compile everywhere.
 */

#define DATA_FILE "students.txt"
#define NAME_LEN 64
#define GRADE_LEN 8

typedef struct {
    int id;
    char name[NAME_LEN];
    int age;
    char grade[GRADE_LEN];
} Student;

// Append a student as a CSV line to the file.
void add_student() {
    Student s;
    printf("Enter ID: ");
    scanf("%d", &s.id);
    printf("Enter Name: ");
    scanf(" %63[^\n]", s.name);  // read full line up to 63 chars
    printf("Enter Age: ");
    scanf("%d", &s.age);
    printf("Enter Grade: ");
    scanf(" %7s", s.grade);

    FILE *f = fopen(DATA_FILE, "a");
    if (!f) { perror("Failed to open file"); return; }
    fprintf(f, "%d,%s,%d,%s\n", s.id, s.name, s.age, s.grade);
    fclose(f);
    printf("Student added.\n");
}

// Read all students from the file and print them.
void list_students() {
    FILE *f = fopen(DATA_FILE, "r");
    if (!f) { printf("No data yet.\n"); return; }

    printf("\n%-6s | %-20s | %-4s | %-6s\n", "ID", "Name", "Age", "Grade");
    printf("-----------------------------------------------------\n");
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        Student s;
        // parse CSV line: id,name,age,grade
        char name[NAME_LEN], grade[GRADE_LEN];
        int id, age;
        if (sscanf(line, "%d,%63[^,],%d,%7s", &id, name, &age, grade) == 4) {
            printf("%-6d | %-20s | %-4d | %-6s\n", id, name, age, grade);
        }
    }
    fclose(f);
}

// Remove a student with matching ID by rewriting the file without that record.
void delete_student() {
    int targetId;
    printf("Enter ID to delete: ");
    scanf("%d", &targetId);

    FILE *in = fopen(DATA_FILE, "r");
    if (!in) { printf("No data file found.\n"); return; }
    FILE *out = fopen("students.tmp", "w");
    if (!out) { perror("Temp file"); fclose(in); return; }

    char line[256];
    int removed = 0;
    while (fgets(line, sizeof(line), in)) {
        int id;
        if (sscanf(line, "%d,", &id) == 1 && id == targetId) {
            removed = 1; // skip writing this line
            continue;
        }
        fputs(line, out);
    }
    fclose(in);
    fclose(out);
    remove(DATA_FILE);
    rename("students.tmp", DATA_FILE);

    if (removed) printf("Student with ID %d removed.\n", targetId);
    else printf("No student with ID %d found.\n", targetId);
}

int main() {
    int choice;
    while (1) {
        printf("\nStudent DB (C - File-Based)\n");
        printf("1. Add Student\n");
        printf("2. List Students\n");
        printf("3. Delete Student\n");
        printf("4. Exit\n");
        printf("Choose: ");
        if (scanf("%d", &choice) != 1) { break; }

        switch (choice) {
            case 1: add_student(); break;
            case 2: list_students(); break;
            case 3: delete_student(); break;
            case 4: printf("Goodbye!\n"); return 0;
            default: printf("Invalid choice.\n");
        }
    }
    return 0;
}
