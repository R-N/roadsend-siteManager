
#
# Table structure for table 'memberSessions'
#

CREATE TABLE memberSessions (
  idxNum SERIAL PRIMARY KEY,
  uID CHAR(32) NOT NULL UNIQUE,
  sID CHAR(32) NOT NULL UNIQUE,
  dateCreated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);


#
# Table structure for table 'members'
#
CREATE TABLE members (
  idxNum SERIAL PRIMARY KEY,
  uID CHAR(32) NOT NULL,
  userName VARCHAR(30) NOT NULL DEFAULT '',
  passWord VARCHAR(30) NOT NULL DEFAULT '',
  emailAddress VARCHAR(60) NOT NULL DEFAULT '',
  firstName VARCHAR(50),
  lastName VARCHAR(50),
  dateCreated TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT unique_email UNIQUE (emailAddress),
  CONSTRAINT unique_uid UNIQUE (uID),
  CONSTRAINT unique_username UNIQUE (userName)
);


#
# Table structure for table 'sessions'
#

CREATE TABLE sessions (
  idxNum SERIAL PRIMARY KEY,
  sessionID VARCHAR(32) NOT NULL DEFAULT '',
  hashID VARCHAR(32) NOT NULL DEFAULT '',
  createStamp TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  remoteHost VARCHAR(20),
  dataKey VARCHAR(255) NOT NULL DEFAULT '',
  dataVal TEXT,
  UNIQUE (hashID)
);
CREATE INDEX createStamp_idx ON sessions (createStamp);
CREATE INDEX sessionID_idx ON sessions (sessionID);
