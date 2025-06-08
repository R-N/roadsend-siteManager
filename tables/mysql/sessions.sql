# MySQL dump 8.8
#
# Host: localhost    Database: test
#--------------------------------------------------------
# Server version	3.23.22-beta-log

#
# Table structure for table 'sessions'
#

CREATE TABLE sessions (
  "idxNum" SERIAL PRIMARY KEY,
  "sessionID" VARCHAR(32) NOT NULL DEFAULT '',
  "hashID" VARCHAR(32) NOT NULL DEFAULT '',
  "createStamp" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "remoteHost" VARCHAR(20),
  "dataKey" VARCHAR(255) NOT NULL DEFAULT '',
  "dataVal" TEXT,
  UNIQUE ("hashID")
);

CREATE INDEX "createStamp_idx" ON sessions ("createStamp");
CREATE INDEX "sessionID_idx" ON sessions ("sessionID");
