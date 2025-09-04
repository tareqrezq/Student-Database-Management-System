#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sqlite3.h>   // Requires libsqlite3-dev at compile time
/*
 * Student Database Management System (C++)
 * ---------------------------------------
 * - OOP design: Student and DatabaseManager classes
 * - RDBMS: SQLite (schema created on startup)
 * - Data Structures: std::vector for in-memory fetch results
 * - Multi-threading: demo concurrent reads using std::thread
 * - Encryption/Decryption: simple XOR-based demo for grade field
 *
 * Build (Linux/Mac):
 *   g++ sdms.cpp -o sdms -lsqlite3 -lpthread
 */

std::mutex coutMutex;

// --- Simple XOR "encryption" demo (for portfolio illustration) ---
std::string xorCipher(const std::string& input, const std::string& key) {
    std::string out = input;
    for (size_t i = 0; i < input.size(); ++i) {
        out[i] = input[i] ^ key[i % key.size()];
    }
    return out;
}

struct Student {
    int id;
    std::string name;
    int age;
    std::string grade; // plaintext in memory, encrypted at rest
};

class DatabaseManager {
private:
    sqlite3* db;
    std::string key; // XOR key
public:
    DatabaseManager(const std::string& dbPath, const std::string& xorKey)
        : db(nullptr), key(xorKey) {
        if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database");
        }
        const char* createSQL =
            "CREATE TABLE IF NOT EXISTS students ("
            " id INTEGER PRIMARY KEY,"
            " name TEXT NOT NULL,"
            " age INTEGER NOT NULL,"
            " grade_enc BLOB NOT NULL"
            ");";
        char* err = nullptr;
        if (sqlite3_exec(db, createSQL, nullptr, nullptr, &err) != SQLITE_OK) {
            std::string e = err ? err : "unknown error";
            sqlite3_free(err);
            throw std::runtime_error("Schema create failed: " + e);
        }
    }

    ~DatabaseManager() {
        if (db) sqlite3_close(db);
    }

    void addStudent(const Student& s) {
        // encrypt grade
        std::string enc = xorCipher(s.grade, key);
        const char* sql = "INSERT INTO students (id, name, age, grade_enc) VALUES (?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("prepare failed");
        }
        sqlite3_bind_int(stmt, 1, s.id);
        sqlite3_bind_text(stmt, 2, s.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, s.age);
        sqlite3_bind_blob(stmt, 4, enc.data(), (int)enc.size(), SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("insert failed");
        }
        sqlite3_finalize(stmt);
    }

    std::vector<Student> getAllStudents() {
        const char* sql = "SELECT id, name, age, grade_enc FROM students ORDER BY id;";
        sqlite3_stmt* stmt = nullptr;
        std::vector<Student> res;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("prepare failed");
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Student s;
            s.id = sqlite3_column_int(stmt, 0);
            s.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            s.age = sqlite3_column_int(stmt, 2);
            const void* blob = sqlite3_column_blob(stmt, 3);
            int len = sqlite3_column_bytes(stmt, 3);
            std::string enc(reinterpret_cast<const char*>(blob), len);
            s.grade = xorCipher(enc, key); // decrypt
            res.push_back(s);
        }
        sqlite3_finalize(stmt);
        return res;
    }

    void updateStudentGrade(int id, const std::string& newGrade) {
        std::string enc = xorCipher(newGrade, key);
        const char* sql = "UPDATE students SET grade_enc=? WHERE id=?;";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_blob(stmt, 1, enc.data(), (int)enc.size(), SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("update failed");
        }
        sqlite3_finalize(stmt);
    }

    void deleteStudent(int id) {
        const char* sql = "DELETE FROM students WHERE id=?;";
        sqlite3_stmt* stmt = nullptr;
        sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, id);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("delete failed");
        }
        sqlite3_finalize(stmt);
    }
};

void printStudents(const std::vector<Student>& v) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "
ID   | Name                 | Age | Grade
";
    std::cout << "----------------------------------------------
";
    for (const auto& s : v) {
        std::cout << std::left << std::setw(4) << s.id << " | "
                  << std::setw(20) << s.name << " | "
                  << std::setw(3) << s.age << " | "
                  << s.grade << "
";
    }
}

int main() {
    try {
        DatabaseManager dbm("students.db", "mySecretKey");

        // Seed example (id 1) if table empty
        auto current = dbm.getAllStudents();
        if (current.empty()) {
            dbm.addStudent({1, "Alice", 20, "A+"});
        }

        int choice;
        while (true) {
            std::cout << "
Student DB (C++ - SQLite/OOP/Threads)
"
                      << "1. Add Student
"
                      << "2. List Students
"
                      << "3. Update Grade
"
                      << "4. Delete Student
"
                      << "5. Concurrent Read Demo
"
                      << "6. Exit
"
                      << "Choose: ";
            if (!(std::cin >> choice)) break;

            if (choice == 1) {
                Student s;
                std::cout << "ID: "; std::cin >> s.id;
                std::cout << "Name: "; std::cin.ignore(); std::getline(std::cin, s.name);
                if (s.name.empty()) std::getline(std::cin, s.name);
                std::cout << "Age: "; std::cin >> s.age;
                std::cout << "Grade: "; std::cin >> s.grade;
                dbm.addStudent(s);
                std::cout << "Added.
";
            } else if (choice == 2) {
                auto v = dbm.getAllStudents();
                printStudents(v);
            } else if (choice == 3) {
                int id; std::string g;
                std::cout << "ID: "; std::cin >> id;
                std::cout << "New Grade: "; std::cin >> g;
                dbm.updateStudentGrade(id, g);
                std::cout << "Updated.
";
            } else if (choice == 4) {
                int id; std::cout << "ID: "; std::cin >> id;
                dbm.deleteStudent(id);
                std::cout << "Deleted.
";
            } else if (choice == 5) {
                // Simple multithreaded read demo
                std::thread t1([&]{ printStudents(dbm.getAllStudents()); });
                std::thread t2([&]{ printStudents(dbm.getAllStudents()); });
                t1.join(); t2.join();
            } else if (choice == 6) {
                break;
            } else {
                std::cout << "Invalid.
";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "
";
        return 1;
    }
    return 0;
}
