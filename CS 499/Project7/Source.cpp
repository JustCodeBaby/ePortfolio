#include <iostream>
#include <sqlite3.h>  // SQLite header for database operations
#include <string>
#include <stdexcept>  // For exception handling

// -------------------- Database Connection Class --------------------
// Manages opening and closing of the SQLite database safely using RAII principles
class DatabaseConnection {
    sqlite3* db;

public:
    // Constructor: Opens a connection to the SQLite database
    DatabaseConnection(const std::string& dbName) {
        if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(db)));
        }
        std::cout << "Database opened successfully!\n";
    }

    // Destructor: Closes the database connection when the object goes out of scope
    ~DatabaseConnection() {
        sqlite3_close(db);
        std::cout << "Database closed successfully!\n";
    }

    // Getter to access the SQLite database pointer
    sqlite3* get() const { return db; }
};

// -------------------- Input Validation --------------------
// Validates input to ensure name and age meet specific requirements
void validateInput(const std::string& name, int age) {
    if (age < 0 || age > 150) {
        throw std::invalid_argument("Age must be between 0 and 150.");
    }
    if (name.empty() || name.length() > 100) {
        throw std::invalid_argument("Name must be between 1 and 100 characters.");
    }
}

// -------------------- Execute Prepared SQL --------------------
// Executes an SQL command using prepared statements to prevent SQL injection
void executePreparedSQL(sqlite3* db, const std::string& sql, const std::string& name, int age) {
    sqlite3_stmt* stmt; // Statement object for prepared SQL execution

    // Prepare the SQL command
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    // Safely bind the parameters to prevent SQL injection
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, age);

    // Execute the SQL command
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::runtime_error("Failed to execute statement: " + std::string(sqlite3_errmsg(db)));
    }

    // Finalize the statement to release resources
    sqlite3_finalize(stmt);
    std::cout << "Operation completed successfully!\n";
}

// -------------------- Create Table --------------------
// Creates the 'Users' table if it does not already exist
void createTable(sqlite3* db) {
    const char* sql = "CREATE TABLE IF NOT EXISTS Users ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "Name TEXT NOT NULL, "
        "Age INTEGER NOT NULL);";

    char* errMsg = nullptr;

    // Execute the SQL command to create the table
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        throw std::runtime_error("Failed to create table: " + std::string(errMsg));
    }

    std::cout << "Table created successfully!\n";
}

// -------------------- Insert Data --------------------
// Inserts a new record into the Users table
void insertData(DatabaseConnection& dbConn, const std::string& name, int age) {
    validateInput(name, age); // Validate input data

    const std::string sql = "INSERT INTO Users (Name, Age) VALUES (?, ?);";
    executePreparedSQL(dbConn.get(), sql, name, age);
}

// -------------------- Read Data --------------------
// Callback function to display query results from the database
static int callback(void* data, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        std::cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    std::cout << "-----------------------" << std::endl;
    return 0;
}

// Reads and displays all records from the Users table
void readData(DatabaseConnection& dbConn) {
    const char* sql = "SELECT * FROM Users;";
    char* errMsg = nullptr;

    if (sqlite3_exec(dbConn.get(), sql, callback, nullptr, &errMsg) != SQLITE_OK) {
        throw std::runtime_error("Failed to read data: " + std::string(errMsg));
    }
}

// -------------------- Update Data --------------------
// Updates an existing record in the Users table based on the user ID
void updateData(DatabaseConnection& dbConn, int id, const std::string& name, int age) {
    validateInput(name, age); // Validate input data

    const char* sql = "UPDATE Users SET Name = ?, Age = ? WHERE ID = ?;";
    sqlite3_stmt* stmt;

    // Prepare the SQL statement
    if (sqlite3_prepare_v2(dbConn.get(), sql, -1, &stmt, NULL) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare update statement: " + std::string(sqlite3_errmsg(dbConn.get())));
    }

    // Bind parameters safely
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, age);
    sqlite3_bind_int(stmt, 3, id);

    // Execute the statement
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::runtime_error("Failed to update data: " + std::string(sqlite3_errmsg(dbConn.get())));
    }

    sqlite3_finalize(stmt);
    std::cout << "Record updated successfully!\n";
}

// -------------------- Main Function --------------------
int main() {
    try {
        // Create database connection 
        DatabaseConnection dbConn("test.db");

        // Create table if it does not exist
        createTable(dbConn.get());

        // Insert sample records
        insertData(dbConn, "Alice", 25);
        insertData(dbConn, "Bob", 30);

        // Display all records
        std::cout << "Current Records:\n";
        readData(dbConn);

        // Update a record
        std::cout << "Updating Bob's age to 35:\n";
        updateData(dbConn, 2, "Bob", 35);

        // Display records after the update
        std::cout << "Records After Update:\n";
        readData(dbConn);

    }
    catch (const std::exception& e) {
        // Handle any runtime exceptions
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}

