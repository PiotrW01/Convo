--CREATE USER terminalus WITH PASSWORD 'password';
-- sudo -u terminalus -d terminal_chat
DO
$$
BEGIN
    IF NOT EXISTS (
        SELECT FROM pg_database WHERE datname = 'terminal_chat'
    ) THEN
        -- replace myuser with the actual user
        CREATE DATABASE terminal_chat OWNER myuser;
    END IF;
END
$$;
