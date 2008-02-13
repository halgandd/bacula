-- --------------------------------------------------
-- Upgrade from 2.2
-- --------------------------------------------------

ALTER TABLE client_group ADD COLUMN comment text;
ALTER TABLE client_group_member RENAME COLUMN clientid TO ClientId;

CREATE TABLE bweb_user
(
	userid       serial not null,
	username     text not null,
	use_acl      boolean default false,
	enabled      boolean default true,
        comment      text default '',
	passwd       text default '',
	primary key (userid)
);
CREATE UNIQUE INDEX bweb_user_idx on bweb_user (username(255));

CREATE TABLE bweb_role
(
	roleid       serial not null,
	rolename     text not null,
	comment      text default '',
	primary key (roleid)
);
CREATE UNIQUE INDEX bweb_role_idx on bweb_role (rolename(255));
INSERT INTO bweb_role (rolename) VALUES ('r_user_mgnt');
INSERT INTO bweb_role (rolename) VALUES ('r_group_mgnt');
INSERT INTO bweb_role (rolename) VALUES ('r_configure');

INSERT INTO bweb_role (rolename) VALUES ('r_autochanger_mgnt');
INSERT INTO bweb_role (rolename) VALUES ('r_location_mgnt');
INSERT INTO bweb_role (rolename) VALUES ('r_delete_job');
INSERT INTO bweb_role (rolename) VALUES ('r_prune');
INSERT INTO bweb_role (rolename) VALUES ('r_purge');

INSERT INTO bweb_role (rolename) VALUES ('r_view_job');
INSERT INTO bweb_role (rolename) VALUES ('r_view_stat');
INSERT INTO bweb_role (rolename) VALUES ('r_view_media');

INSERT INTO bweb_role (rolename) VALUES ('r_run_job');
INSERT INTO bweb_role (rolename) VALUES ('r_cancel_job');
INSERT INTO bweb_role (rolename) VALUES ('r_client_status');

CREATE TABLE  bweb_role_member
(
	roleid       integer not null,
	userid       integer not null,
	primary key (roleid, userid)
);

CREATE TABLE  bweb_client_group_acl
(
	client_group_id       integer not null,
	userid                integer not null,
	primary key (client_group_id, userid)
);
