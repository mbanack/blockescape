DROP TABLE users;
DROP TABLE highscore;
DROP PROCEDURE IF EXISTS POPULATE;
CREATE TABLE users (id MEDIUMINT NOT NULL AUTO_INCREMENT, username TEXT NOT NULL, password TEXT NOT NULL, salt TEXT NOT NULL, levels TEXT NOT NULL, hints MEDIUMINT NOT NULL, PRIMARY KEY(id));
CREATE TABLE highscore (bid INT NOT NULL, topUser TEXT NOT NULL, topMoves TEXT NOT NULL, topTime TEXT NOT NULL, topUser2 TEXT NOT NULL, topMoves2 TEXT NOT NULL, topTime2 TEXT NOT NULL, topUser3 TEXT NOT NULL, topMoves3 TEXT NOT NULL, topTime3 TEXT NOT NULL, PRIMARY KEY(bid)); 
DELIMITER $$
CREATE PROCEDURE POPULATE()
    BEGIN
        DECLARE countervar INT unsigned default 0;
        while countervar < 1025 do
            insert into highscore values(countervar, "ABC", "9999", "9999", "ABC", "9999", "9999", "ABC", "9999", "9999");
            SET countervar = countervar + 1;
        END WHILE;
    commit;
END $$
DELIMITER ;
CALL POPULATE();
