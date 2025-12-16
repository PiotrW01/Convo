#!/bin/bash
set -e

echo "Do you want to install MariaDB server and client? [y/N]"
read -r install_db

sudo apt update

# compiler, build tools, and CMake
sudo apt install -y build-essential gcc-12 g++-12 cmake

# openSSL
sudo apt install -y libssl-dev

# database libs
sudo apt install -y libmariadb3 libmariadb-dev

if [[ "$install_db" =~ ^[Yy]$ ]]; then
    sudo apt install -y mariadb-server mariadb-client
else
    echo "Skipping MariaDB server and client installation"
fi

if ! sudo apt install -y mariadb-connector-cpp; then
echo "Installing mariadb-connector-cpp failed"
echo "Follow MariaDB Connector/C++ instructions to build and install the connector instead:"
echo "https://mariadb.com/docs/connectors/mariadb-connector-cpp/install-mariadb-connector-cpp"
fi

echo "Project initialization completed"