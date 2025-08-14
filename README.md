# Hospital Bed & Resource Allocation System (C++ • MySQL • Heaps & Queues)

## Overview
A hospital operations tool that allocates beds to patients by urgency using a priority queue, tracks critical medical resources, and generates daily reports. Designed as a demo-ready C++ application with a MySQL backend.

## Files
- `schema.sql` — create database `hospital_alloc` and sample data
- `main.cpp` — C++ CLI application (MySQL C API)
- `README.md` — this file

## Features
- Priority-based bed allocation (higher urgency patients served first)
- Resource reservation (Ventilator, Oxygen Cylinder, etc.) via DB and in-memory hash map
- Daily reports: occupied beds, unallocated patients, resource inventory
- CLI to add patients, beds, and resources

## Setup (Windows)
1. Install **MySQL Server** and note credentials.
2. Import schema: 
   ```bat
   mysql -u root -p < path\to\schema.sql
   ```
3. Edit `main.cpp` and set your MySQL `pass` (search for `pass = "password"`).
4. Build with **MSVC**:
   ```bat
   cl /EHsc main.cpp /I"C:\Program Files\MySQL\MySQL Server 8.0\include" /link /LIBPATH:"C:\Program Files\MySQL\MySQL Server 8.0\lib" libmysql.lib
   ```
   Or build with **MinGW/g++**:
   ```bat
   g++ main.cpp -o hospital.exe -I"C:\Program Files\MySQL\MySQL Server 8.0\include" -L"C:\Program Files\MySQL\MySQL Server 8.0\lib" -lmysql
   ```
5. Ensure `libmysql.dll` is on PATH or next to the executable.

## Run
```bat
hospital.exe
```
Then choose options from the CLI menu to allocate beds, add patients, and generate reports.

