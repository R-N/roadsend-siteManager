#
# Table structure for table 'members'
#
CREATE TABLE members (
  "idxNum" SERIAL PRIMARY KEY,
  "uID" CHAR(32) NOT NULL,
  "userName" VARCHAR(30) NOT NULL DEFAULT '',
  "passWord" VARCHAR(30) NOT NULL DEFAULT '',
  "emailAddress" VARCHAR(60) NOT NULL DEFAULT '',
  "firstName" VARCHAR(50),
  "lastName" VARCHAR(50),
  "dateCreated" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE ("emailAddress"),
  UNIQUE ("uID"),
  UNIQUE ("userName")
);
