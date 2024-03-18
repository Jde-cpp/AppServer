drop procedure if exists log_application_instance_insert;

DELIMITER $$;
CREATE  PROCEDURE log_application_instance_insert( _application_name varchar(255), _host_name varchar(255), _process_id int unsigned )
begin
	declare _host_id int unsigned;
	declare _application_id int unsigned;
	declare _instance_id int unsigned;
   declare _db_log_level int unsigned;
   declare _file_log_level int unsigned;
   select id, db_log_level, file_log_level into _application_id,  _db_log_level, _file_log_level from log_applications where name=_application_name;
   if( _application_id is null ) then
		call log_application_insert( _application_name, _application_id );
   end if;

	select id into _host_id from log_hosts where name=_host_name;
   if( _host_id is null )  then
		insert into log_hosts( name ) values( _host_name );
		select LAST_INSERT_ID() into _host_id;
   end if;

	insert into log_application_instances(application_id,end_time,host_id,process_id,start) values( _application_id,null,_host_id,_process_id,NOW() );
	select LAST_INSERT_ID() into _instance_id;
   select _application_id, _instance_id, _db_log_level, _file_log_level;
end
$$
DELIMITER ;