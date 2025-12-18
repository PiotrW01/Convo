# Convo

**Convo** is a terminal-based chat application written entirely in C++. The project consists of a **client** and a **server**. 
This project serves as both a learning experience in secure network programming and a foundation for building more complex systems.

---

## Features

- Client and server written in C++  
- Full terminal UI using FTXUI library  
- Secure communication via SSL/TLS  
- Custom chat protocol  
- User registration (`/register`) and login (`/login`)  
- Sending and receiving messages in real-time  

---

## Key Libraries / Dependencies

The project relies on the following libraries:

- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) - For building a full terminal UI for the client

- [ASIO](https://think-async.com/Asio/) - For asynchronous networking (TCP sockets)

- [OpenSSL](https://openssl-library.org/) - Provides SSL/TLS encryption for secure communication

- [fmt](https://github.com/fmtlib/fmt) - Formatting strings safely and efficiently

- [libbcrypt](https://github.com/trusch/libbcrypt) - Password hashing for user authentication

- [MariaDB](https://mariadb.org/) - Database and c++ connector

## Prerequisites

Before building the project, run the following scripts to install dependencies and configure the database:

```bash
./00_INIT.sh
./01_CONF_DB.sh
```

## Building the Project

Use the build.sh script to compile the project:
```bash
# Build both client and server
./build.sh

# Build only the client
./build.sh client

# Build only the server
./build.sh server
```

## Running the client

```bash
  -h, --help            Show this help and exit
  -p, --port <port>     Server port (default: {})
  -i, --ip   <address>  Server IP address (default: {})

 ./client [OPTIONS]
```

## Running the server

```bash
  -h, --help            Show this help and exit
  -p, --port <port>     Server port (default: {})
  -dA <address>         Database address (default: {})
  -dU <user>            Database user (default: {})
  -dP <password>        Database password
  -dS <schema>          Database schema (default: {})
  -cP <path>            TLS certificate PEM path
  -cK <path>            TLS certificate private key path

  ./server [OPTIONS]
```

## Environment Variables
The server also supports configuration through environment variables:

```bash
  SERVER_PORT
  DB_ADDRESS
  DB_USER
  DB_PASSWORD
  DB_SCHEMA
  CERT_PEM_PATH
  CERT_KEY_PATH
```

## Usage example
  
### Starting the server
```bash
  ./build/server/server -p 12345 -dA localhost -dU user -dP password -dS convo_db -cP cert/server.pem -cK cert/server.key
```
### Starting the client
```bash
  ./build/client/client -i 127.0.0.1 -p 12345
```
### Register a new user
```bash
  /register myusername mypassword
```
### Login
```bash
  /login myusername mypassword
```
## Screenshots
<img width="1050" height="624" alt="Screenshot from 2025-12-18 18-00-16" src="https://github.com/user-attachments/assets/e65b9d74-60e3-4fc6-af38-caebfec1a1e4" />

---

<img width="1474" height="762" alt="Screenshot from 2025-12-18 18-02-01" src="https://github.com/user-attachments/assets/7db0b2dd-2c28-4464-b17a-c706ba70afb4" />

