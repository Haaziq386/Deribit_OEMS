# Order Execution and Management System for Deribit

---

## Screenshots

![OEMS](https://github.com/user-attachments/assets/5e47e40d-7f26-4504-8900-cf02844ac07a)
![Websocket CLient](https://github.com/user-attachments/assets/1e1d8fa5-d317-467c-83cb-6f9b1f74dfde)
![Websocket Server](https://github.com/user-attachments/assets/8c3743a8-0fab-4212-92ac-c12714f64181)


---

## Video Demo

https://drive.google.com/file/d/1VEZIMYgDX3RalEK3iySwAn7Rg99RjZQU/view?usp=sharing

---

## Features

- Place, modify, and cancel orders seamlessly.
- Retrieve real-time order book and positions.
- View open orders and trade history.
- Authenticate and connect to Deribit’s WebSocket server.
- Monitor CPU and memory usage.

---

## Tech Stack

- **Programming Language:** C++
- **Build System:** CMake

### Libraries Used

#### Core Libraries
- **simdjson:** For efficient JSON parsing.
- **libcurl:** For REST API interactions.

#### Additional Libraries
- **C++ Standard Libraries:** Used for threading, file I/O, and data structures.

---

## Usage

1. Clone this repository:
   ```bash
   git clone <repository-url>
   cd <repository-directory>
   ```

2. Build the application:
   ```bash
   mkdir build
   cd build/
   cmake ..
   make
   ```

3. Run the application:
   ```bash
   ./deribit_order_management
   ```

4. For the server:
   ```bash
   cd server/
   g++ websocket_server.cpp utils.cpp threadpool.cpp -lcurl
   ./a.out
   ```

---

## Code Structure

The app follows a modular code structure for maintainability and scalability:

### Directory Structure

```
.
├── include/           # Header files
├── src/               # Source files
├── server/            # WebSocket server code
├── build/             # Build directory
├── CMakeLists.txt     # Build configuration
└── README.md          # Documentation
```

### Design Choices

- **Modular Components:** Separate files for order management, WebSocket client, and utility functions.
- **Real-time Data:** Simple and responsive UI.
- **Error Handling:** Robust exception handling for JSON parsing and network communication.

---

## Challenges and Solutions

### Few Bugs Fixed
- **JSON Parsing Errors:** Resolved issues with handling malformed responses using simdjson.
- **WebSocket Threading:** Addressed threading conflicts during real-time data updates.

### Other Limitations
- Limited testing on non-Linux platforms.
- Requires manual server setup for WebSocket functionality.
- Requires good internet speed
---

## Made with ❤️ for intern assignmnet of GoQuant

