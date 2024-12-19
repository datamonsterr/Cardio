create database cardio;
DROP TABLE IF EXISTS player, ranking, friend, room, match, host, seat, history;

CREATE TABLE player (
    player_id int PRIMARY KEY,
    fullname varchar,
    gender char,
    date_of_birth varchar,
    age int,
    country varchar,
    password varchar,
    avatar bytea,
    balance float,
    rank int,
    registration_date varchar
);

CREATE TABLE ranking (
    balance float,
    player_id int PRIMARY KEY,
    CONSTRAINT fk_ranking_player FOREIGN KEY (player_id) REFERENCES player (player_id)
        ON DELETE CASCADE
);

CREATE TABLE friend (
    friendship_id int PRIMARY KEY,
    player1_id int,
    player2_id int,
    friend_time timestamp,
    CONSTRAINT fk_friend_player1 FOREIGN KEY (player1_id) REFERENCES player (player_id),
    CONSTRAINT fk_friend_player2 FOREIGN KEY (player2_id) REFERENCES player (player_id)
);

CREATE TABLE room (
    room_id int PRIMARY KEY,
    min_player int,
    max_player int,
    host_id int,
    status varchar CHECK (status IN ('exist', 'not exist')),
    CONSTRAINT fk_room_host FOREIGN KEY (host_id) REFERENCES player (player_id) ON UPDATE CASCADE
);

CREATE TABLE match (
    match_id int PRIMARY KEY,
    room_id int,
    winner int,  -- player_id
    status varchar CHECK (status IN ('progress', 'finish')),
    match_date date,
    start_time time,
    end_time time,
    progress bytea,  -- 64KB
    CONSTRAINT fk_match_room FOREIGN KEY (room_id) REFERENCES room (room_id) ON DELETE CASCADE,
    CONSTRAINT fk_match_winner FOREIGN KEY (winner) REFERENCES player (player_id)
);

CREATE TABLE host (
    host_id int PRIMARY KEY,
    CONSTRAINT fk_host_player FOREIGN KEY (host_id) REFERENCES player (player_id) ON UPDATE CASCADE
);

CREATE TABLE seat (
    seat_number int PRIMARY KEY,
    player_id int,
    room_id int,
    sit varchar CHECK (sit IN ('yes', 'no')),
    CONSTRAINT fk_seat_player FOREIGN KEY (player_id) REFERENCES player (player_id) 
        ON DELETE CASCADE ON UPDATE CASCADE,
    CONSTRAINT fk_seat_room FOREIGN KEY (room_id) REFERENCES room (room_id) 
        ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE history (
    match_id int,
    CONSTRAINT fk_history_match FOREIGN KEY (match_id) REFERENCES match (match_id)
);


