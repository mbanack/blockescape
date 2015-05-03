DROP TABLE users;
CREATE TABLE users (username CHAR(128), password TEXT NOT NULL, salt TEXT NOT NULL, levels TEXT NOT NULL, PRIMARY KEY(username));
