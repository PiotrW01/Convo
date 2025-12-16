#!/bin/bash
set -e

if ! command -v mysql &> /dev/null; then
    echo "MariaDB client not found. Please install mariadb-client first."
    exit 1
fi

CREATE_DB_SQL="db/00_create-db.sql"
SCHEMA_SQL="db/01_schema.sql"
SEED_SQL="db/02_seed.sql"

# Check if scripts exist
for file in "$CREATE_DB_SQL" "$SCHEMA_SQL" "$SEED_SQL"; do
    if [ ! -f "$file" ]; then
        echo "SQL script not found: $file"
        exit 1
    fi
done

echo "Running $CREATE_DB_SQL..."
sudo mysql < "$CREATE_DB_SQL"

echo "Running $SCHEMA_SQL..."
sudo mysql < "$SCHEMA_SQL"

echo "Running $SEED_SQL..."
sudo mysql < "$SEED_SQL"

echo "Database setup completed"
