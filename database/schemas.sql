CREATE TABLE "User"
(
    user_id  SERIAL PRIMARY KEY,
    email    VARCHAR(100) NOT NULL UNIQUE,
    phone    VARCHAR(15)  NOT NULL,
    dob      DATE         NOT NULL,
    password VARCHAR(128) NOT NULL,
    country  VARCHAR(32),
    gender VARCHAR(10) NOT NULL CHECK ( gender IN ('Male', 'Female', 'Other') ),
    balance  INTEGER DEFAULT 0,
    registration_date DATE DEFAULT CURRENT_DATE,
    avatar_url VARCHAR(100),
    full_name VARCHAR(100) NOT NULL DEFAULT '',
    username VARCHAR(100) NOT NULL UNIQUE
);

CREATE TABLE Friend
(
    u1 INTEGER NOT NULL,
    u2 INTEGER NOT NULL,
    FOREIGN KEY (u1) references "User"(user_id),
    FOREIGN KEY (u2) references "User"(user_id),
    PRIMARY KEY (u1, u2)
);

SELECT * FROM "User";