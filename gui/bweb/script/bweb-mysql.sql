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
--	comment      text default '',
	primary key (roleid)
);
CREATE UNIQUE INDEX bweb_role_idx on bweb_role (rolename(255));
INSERT INTO bweb_role (rolename) VALUES ('r_user_mgnt');
INSERT INTO bweb_role (rolename) VALUES ('r_delete_job');
INSERT INTO bweb_role (rolename) VALUES ('r_prune');
INSERT INTO bweb_role (rolename) VALUES ('r_purge');
INSERT INTO bweb_role (rolename) VALUES ('r_group_mgnt');
INSERT INTO bweb_role (rolename) VALUES ('r_location_mgnt');
INSERT INTO bweb_role (rolename) VALUES ('r_cancel_job');
INSERT INTO bweb_role (rolename) VALUES ('r_run_job');
INSERT INTO bweb_role (rolename) VALUES ('r_configure');
INSERT INTO bweb_role (rolename) VALUES ('r_client_status');
INSERT INTO bweb_role (rolename) VALUES ('r_view_job');

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


-- Manage Client groups in bweb
-- Works with postgresql and mysql5 

CREATE TABLE client_group
(
    client_group_id             serial	  not null,
    client_group_name	        text	  not null,
    primary key (client_group_id)
);

CREATE UNIQUE INDEX client_group_idx on client_group (client_group_name(255));

CREATE TABLE client_group_member
(
    client_group_id 		    integer	  not null,
    clientid          integer     not null,
    primary key (client_group_id, clientid)
);

CREATE INDEX client_group_member_idx on client_group_member (client_group_id);


--   -- creer un nouveau group
--   
--   INSERT INTO client_group (client_group_name) VALUES ('SIGMA');
--   
--   -- affecter une machine a un group
--   
--   INSERT INTO client_group_member (client_group_id, clientid) 
--          (SELECT client_group_id, 
--                 (SELECT Clientid FROM Client WHERE Name = 'slps0003-fd')
--             FROM client_group 
--            WHERE client_group_name IN ('SIGMA', 'EXPLOITATION', 'MUTUALISE'));
--        
--   
--   -- modifier l'affectation d'une machine
--   
--   DELETE FROM client_group_member 
--         WHERE clientid = (SELECT ClientId FROM Client WHERE Name = 'slps0003-fd')
--   
--   -- supprimer un groupe
--   
--   DELETE FROM client_group_member 
--         WHERE client_group_id = (SELECT client_group_id FROM client_group WHERE client_group_name = 'EXPLOIT')
--   
--   
--   -- afficher tous les clients du group SIGMA
--   
--   SELECT Name FROM Client JOIN client_group_member using (clientid) 
--                           JOIN client_group using (client_group_id)
--    WHERE client_group_name = 'SIGMA';
--   
--   -- afficher tous les groups
--   
--   SELECT client_group_name FROM client_group ORDER BY client_group_name;
--   
--   -- afficher tous les job du group SIGMA hier
--   
--   SELECT JobId, Job.Name, Client.Name, JobStatus, JobErrors
--     FROM Job JOIN Client              USING(ClientId) 
--              JOIN client_group_member USING (ClientId)
--              JOIN client_group        USING (client_group_id)
--     WHERE client_group_name = 'SIGMA'
--       AND Job.StartTime > '2007-03-20'; 
--   
--   -- donne des stats
--   
--   SELECT count(1) AS nb, sum(JobFiles) AS files,
--          sum(JobBytes) AS size, sum(JobErrors) AS joberrors,
--          JobStatus AS jobstatus, client_group_name
--     FROM Job JOIN Client              USING(ClientId) 
--              JOIN client_group_member USING (ClientId)
--              JOIN client_group        USING (client_group_id)
--     WHERE Job.StartTime > '2007-03-20'
--     GROUP BY JobStatus, client_group_name
--   
--   
