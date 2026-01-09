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
-- Friend invites table
CREATE TABLE IF NOT EXISTS friend_invites
(
    invite_id SERIAL PRIMARY KEY,
    from_user_id INTEGER NOT NULL,
    to_user_id INTEGER NOT NULL,
    status VARCHAR(16) NOT NULL DEFAULT 'pending' CHECK (status IN ('pending', 'accepted', 'rejected')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (from_user_id) REFERENCES "User"(user_id) ON DELETE CASCADE,
    FOREIGN KEY (to_user_id) REFERENCES "User"(user_id) ON DELETE CASCADE,
    UNIQUE (from_user_id, to_user_id)
);

CREATE INDEX idx_friend_invites_to_user ON friend_invites(to_user_id) WHERE status = 'pending';
CREATE INDEX idx_friend_invites_from_user ON friend_invites(from_user_id);
