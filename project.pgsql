--
-- Table structure for table 'project_projects'
--

CREATE TABLE project_projects (
  nid int NOT NULL default '0',
  uri varchar(50) NOT NULL default '',
  homepage varchar(255) NOT NULL default '',
  changelog varchar(255) NOT NULL default '',
  cvs varchar(255) NOT NULL default '',
  demo varchar(255) NOT NULL default '',
  release_directory varchar(255) NOT NULL default '',
  issues smallint NOT NULL default '0',
  components text,
  version int NOT NULL default '0',
  mail varchar(255) not null default '',
  mail_digest varchar(255) not null default '',
  mail_copy varchar(255) not null default '',
  mail_copy_filter varchar(255) not null default '',
  mail_reminder smallint NOT NULL default '0',
  help text,
  screenshots varchar(255) default '' not null,
  mail_copy_filter_state varchar(255) default '' not null,
  documentation varchar(255) default '' not null,
  license varchar(255) default '' not null,
  PRIMARY KEY (nid)
);
CREATE INDEX project_projects_uri_idx ON project_projects(uri);

--
-- Table structure for table 'project_releases'
--

CREATE TABLE project_releases (
  rid int NOT NULL default '0',
  nid int NOT NULL default '0',
  fid int NOT NULL default '0',
  path varchar(255) NOT NULL default '',
  created int NOT NULL default '0',
  version varchar(255) NOT NULL default '',
  changes text,
  weight smallint NOT NULL default '0',
  changed int NOT NULL default '0',
  status smallint default '1' not null,
  PRIMARY KEY (rid)
);
CREATE INDEX project_releases_nid_idx ON project_releases(nid);


--
-- Table structure for table 'project_issues'
--

CREATE TABLE project_issues (
  nid int NOT NULL default '0',
  pid int NOT NULL default '0',
  category varchar(255) NOT NULL default '',
  component varchar(255) NOT NULL default '',
  priority smallint NOT NULL default '0',
  rid int NOT NULL default '0',
  assigned int NOT NULL default '0',
  state smallint NOT NULL default '0',
  file_path varchar(255) NOT NULL default '',
  file_mime varchar(255) default '' NOT NULL,
  file_size int default 0 NOT NULL,
  PRIMARY KEY (nid)
);
CREATE INDEX project_issues_pid_idx ON project_issues(pid);


--
-- Table structure for table 'project_comments'
--

CREATE TABLE project_comments (
  cid int NOT NULL default '0',
  nid int NOT NULL default '0',
  uid int NOT NULL default '0',
  name varchar(255) NOT NULL default '',
  created int NOT NULL default '0',
  changed int NOT NULL default '0',
  body bytea,
  data bytea,
  file_path varchar(255) default '' NOT NULL,
  file_mime varchar(255) default '' NOT NULL,
  file_size int default 0 NOT NULL,
  PRIMARY KEY (cid)
);
CREATE INDEX project_comments_nid_idx ON project_comments(nid);


--
-- Table structure for table 'project_subscriptions'
--

CREATE TABLE project_subscriptions (
  nid int NOT NULL default '0',
  uid int NOT NULL default '0',
  level smallint NOT NULL default '0'
);
CREATE INDEX project_subscriptions_nic_uid_level_idx ON project_subscriptions(nid, uid, level);

CREATE SEQUENCE project_cid_seq INCREMENT 1 START 1;
CREATE SEQUENCE project_rid_seq INCREMENT 1 START 1;
