create user ${USER};

-- for tables
grant all on unsavedfiles to ${USER};
grant all on basefiles	  to ${USER};
grant all on jobmedia	  to ${USER};
grant all on file	  to ${USER};
grant all on job	  to ${USER};
grant all on media	  to ${USER};
grant all on client	  to ${USER};
grant all on pool	  to ${USER};
grant all on fileset	  to ${USER};
grant all on path	  to ${USER};
grant all on filename	  to ${USER};
grant all on counters	  to ${USER};
grant all on version	  to ${USER};
grant all on cdimages	  to ${USER};
grant all on mediatype	  to ${USER};
grant all on storage	  to ${USER};
grant all on device	  to ${USER};
grant all on status	  to ${USER};

-- for sequences on those tables

grant select, update on filename_filenameid_seq    to ${USER};
grant select, update on path_pathid_seq 	   to ${USER};
grant select, update on fileset_filesetid_seq	   to ${USER};
grant select, update on pool_poolid_seq 	   to ${USER};
grant select, update on client_clientid_seq	   to ${USER};
grant select, update on media_mediaid_seq	   to ${USER};
grant select, update on job_jobid_seq		   to ${USER};
grant select, update on file_fileid_seq 	   to ${USER};
grant select, update on jobmedia_jobmediaid_seq    to ${USER};
grant select, update on basefiles_baseid_seq	   to ${USER};
grant select, update on storage_storageid_seq	   to ${USER};
grant select, update on mediatype_mediatypeid_seq  to ${USER};
grant select, update on device_deviceid_seq	   to ${USER};
