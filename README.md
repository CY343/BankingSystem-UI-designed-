 # C++ Qt Banking System

A desktop banking system built with modern C++ (C++17) and Qt 6, designed to model real-world banking operations using clean object-oriented architecture, strong separation of concerns, and extensible design.

The application supports role-based authentication, account management, and debit/credit card handling, all exposed through a graphical user interface built with Qt Widgets.

---

## Overview

This project simulates a simplified banking environment where customers can manage multiple accounts and cards, while managers can oversee system operations. The system emphasizes maintainability, correctness, and scalability, reflecting real software engineering practices rather than a minimal prototype.

The core business logic is intentionally decoupled from the UI and data layer, allowing the application to be extended with additional storage backends or interfaces without modifying domain logic.

---

## Key Features

- Role-based authentication (Manager and Customer roles)
- Checking and Saving account support with enforced business rules
- Debit and Credit card issuance and management
- Customers can own multiple accounts and cards
- GUI-driven navigation built with Qt 6 Widgets
- Clean separation between UI, business logic, and data handling

---

## Architecture & Design

The application follows a layered architecture:

- UI Layer (Qt Widgets): Handles user interaction and presentation
- Service Layer: Coordinates application logic and enforces business rules
- Domain Layer: Core entities such as accounts, cards, customers, and transactions
- Data Layer: Interface-based design allowing flexible data storage implementations

### Design Principles

- Object-Oriented Programming (inheritance and polymorphism)
- RAII and smart pointer usage (`std::shared_ptr`)
- Interface-based architecture for extensibility
- Single Responsibility Principle
- Exception-based error handling

---

## Tech Stack

- Language: C++17  
- GUI Framework: Qt 6 (Widgets)  
- Build System: CMake  
- Compiler: MSVC 2022  
- IDE: Qt Creator / Visual Studio 2022  

---

### Customer onboarding is manager-controlled (customers are created by Manager role; no public self-signup)

## Build & Run Instructions (Windows)

### Prerequisites

- Qt 6.x (MSVC 64-bit)
- CMake 3.19 or newer
- Visual Studio 2022 (MSVC v143)

### Build Steps

```bash
git clone https://github.com/YOUR_USERNAME/cpp-qt-banking-system.git
cd cpp-qt-banking-system
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Debug

```
When running outside Qt Creator, use `windeployqt` to deploy required Qt runtime libraries.

---
## Screenshots

### Login
![Login Screen](docs/screenshots/login.png)

### Customer Dashboard
![Customer Dashboard](docs/screenshots/customerDashboard.png)

### Manager Dashboard
![Manager Dashboard](docs/screenshots/managerDashboard.png)
---

## 🎥 Demo

▶️ **Watch the demo video:**  
https://github.com/CY343/cpp-qt-banking-system/releases/tag/v1.0-demo

This video demonstrates:
- Customer login and dashboard
- Account creation (Savings / Checking)
- Deposits & withdrawals with validation
- Debit card daily limits
- Transaction history export (CSV / PDF)
- Manager dashboard for customer administration

  
## Author

Cheng Yue  
Computer Science Student  
Interests: C++, Object-Oriented Design, Systems Programming
