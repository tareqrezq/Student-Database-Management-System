"""
Student Database Management System (Python)
------------------------------------------
- OOP design: Student, DatabaseManager
- RDBMS: SQLite
- Encryption/Decryption: Fernet (symmetric AES-GCM under the hood)
- Multi-threading: parallel read demo via threading
"""

import os
import sqlite3
from dataclasses import dataclass
from typing import List
from threading import Thread, Lock

try:
    from cryptography.fernet import Fernet
    CRYPTO_OK = True
except Exception:
    # If cryptography isn't installed, we degrade gracefully by base64 "encoding".
    import base64
    CRYPTO_OK = False

PRINT_LOCK = Lock()

@dataclass
class Student:
    id: int
    name: str
    age: int
    grade: str  # plaintext in memory


class DatabaseManager:
    def __init__(self, db_path: str, key_path: str = "secret.key"):
        self.db_path = db_path
        self.conn = sqlite3.connect(self.db_path, check_same_thread=False)
        self.conn.execute("""
            CREATE TABLE IF NOT EXISTS students (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                age INTEGER NOT NULL,
                grade_enc BLOB NOT NULL
            )
        """)
        self.conn.commit()

        # Load or generate key for encryption.
        self.fernet = None
        if CRYPTO_OK:
            if not os.path.exists(key_path):
                with open(key_path, "wb") as f:
                    f.write(Fernet.generate_key())
            with open(key_path, "rb") as f:
                self.fernet = Fernet(f.read())

    def _enc(self, s: str) -> bytes:
        if self.fernet:
            return self.fernet.encrypt(s.encode())
        else:
            # fallback for environments without cryptography (NOT secure)
            import base64
            return base64.b64encode(s.encode())

    def _dec(self, b: bytes) -> str:
        if self.fernet:
            return self.fernet.decrypt(b).decode()
        else:
            import base64
            return base64.b64decode(b).decode()

    def add_student(self, s: Student):
        self.conn.execute(
            "INSERT INTO students(id, name, age, grade_enc) VALUES(?, ?, ?, ?)",
            (s.id, s.name, s.age, self._enc(s.grade)),
        )
        self.conn.commit()

    def list_students(self) -> List[Student]:
        cur = self.conn.execute("SELECT id, name, age, grade_enc FROM students ORDER BY id")
        res = []
        for row in cur.fetchall():
            res.append(Student(id=row[0], name=row[1], age=row[2], grade=self._dec(row[3])))
        return res

    def update_grade(self, student_id: int, new_grade: str):
        self.conn.execute(
            "UPDATE students SET grade_enc=? WHERE id=?",
            (self._enc(new_grade), student_id),
        )
        self.conn.commit()

    def delete_student(self, student_id: int):
        self.conn.execute("DELETE FROM students WHERE id=?", (student_id,))
        self.conn.commit()


def print_students(students: List[Student]):
    with PRINT_LOCK:
        print("\nID   | Name                 | Age | Grade")
        print("----------------------------------------------")
        for s in students:
            print(f"{s.id:<4} | {s.name:<20} | {s.age:<3} | {s.grade}")


def concurrent_read_demo(db: DatabaseManager):
    t1 = Thread(target=lambda: print_students(db.list_students()))
    t2 = Thread(target=lambda: print_students(db.list_students()))
    t1.start(); t2.start()
    t1.join(); t2.join()


def main():
    db = DatabaseManager("students.db")
    # Seed example if empty
    if not db.list_students():
        db.add_student(Student(1, "Alice", 20, "A+"))

    while True:
        print("\nStudent DB (Python - SQLite/OOP/Threads/Encryption)")
        print("1. Add Student")
        print("2. List Students")
        print("3. Update Grade")
        print("4. Delete Student")
        print("5. Concurrent Read Demo")
        print("6. Exit")
        choice = input("Choose: ").strip()

        try:
            if choice == "1":
                sid = int(input("ID: "))
                name = input("Name: ").strip()
                age = int(input("Age: "))
                grade = input("Grade: ").strip()
                db.add_student(Student(sid, name, age, grade))
                print("Added.")
            elif choice == "2":
                print_students(db.list_students())
            elif choice == "3":
                sid = int(input("ID: "))
                grade = input("New Grade: ").strip()
                db.update_grade(sid, grade)
                print("Updated.")
            elif choice == "4":
                sid = int(input("ID: "))
                db.delete_student(sid)
                print("Deleted.")
            elif choice == "5":
                concurrent_read_demo(db)
            elif choice == "6":
                break
            else:
                print("Invalid choice.")
        except Exception as e:
            print("Error:", e)


if __name__ == "__main__":
    main()
