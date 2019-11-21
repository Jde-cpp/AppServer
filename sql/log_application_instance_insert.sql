


drop procedure log_application_instance_insert2;
DELIMITER $$
CREATE  PROCEDURE log_application_instance_insert2( _application_name varchar(255), _host_name varchar(255), _process_id int unsigned )
begin
	declare _host_id int unsigned; 
	declare _application_id int unsigned; 
	declare _instance_id int unsigned;
   declare _db_log_level int unsigned;
   declare _file_log_level int unsigned;
	#select id into _application_id, db_log_level into _db_log_level, file_log_level into _file_log_level from log_applications where name=_application_name;
   select id, db_log_level, file_log_level into _application_id,  _db_log_level, _file_log_level from log_applications where name=_application_name;
   if( _application_id is null ) then
		call log_application_insert( _application_name, _application_id );
   end if;

	select id into _host_id from log_hosts where name=_host_name;
   if( _host_id is null )  then
		call log_host_insert( _host_name, _host_id );
   end if;
   
	insert into log_application_instances(application_id,end,host_id,process_id,start) values( _application_id,null,_host_id,_process_id,NOW() );
	select LAST_INSERT_ID() into _instance_id;
   select _application_id, _instance_id, _db_log_level, _file_log_level;
end$$
DELIMITER ;

use logs;
call test_proc
DELIMITER $$
CREATE  PROCEDURE test_proc()
begin
   select 0, 0;
end$$
DELIMITER ;


#drop procedure log_application_instance_insert;
#C:\code\7.0\Market\DB\sql\log_application_instance_insert.sql 
DELIMITER $$
CREATE  PROCEDURE `log_application_instance_insert`( _application_name varchar(255), _host_name varchar(255), _process_id int unsigned, out _application_id int unsigned, out _instance_id int unsigned )
begin
	declare _host_id int unsigned;
	select id into _application_id from log_applications where name=_application_name;
   	if( _application_id is null ) then
		call log_application_insert( _application_name, _application_id );
   end if;

	select id into _host_id from log_hosts where name=_host_name;
   if( _host_id is null )  then
		call log_host_insert( _host_name, _host_id );
   end if;
   
	insert into log_application_instances(application_id,end,host_id,process_id,start) values( _application_id,null,_host_id,_process_id,NOW() );
	select LAST_INSERT_ID() into _instance_id;
end$$
DELIMITER ;

use logs;
insert into log_messages(application_id,id,value)values(1,1353503722,'Created lo')
delete from log_messages;
select * from log_messages
select * from log_applications
select * from log_application_instances

#declare _id int unsigned;
#call log_application_instance_insert('myapp', 'localhost', 888, @_id);
#select @_id

#SELECT * FROM logs.log_application_instances;
SELECT * FROM log_hosts;
SELECT * FROM log_applications;
SELECT * FROM log_files;
SELECT * FROM log_messages 

SELECT time local_time, CONVERT_TZ(time, @@session.time_zone, '+00:00') utc
select *
FROM logs
order by time desc
limit 10 

#select * from log_application_instances

#insert into log_application_instances(application_id,end,host_id,process_id,start)
#		values( 0,null,1,2,NOW() );

_application_name varchar(255), _host_name varchar(255), _process_id int unsigned, out _application_id int unsigned, out _instance_id
call logs.log_application_instance_insert( 'adhoc','PL1USPMU0029WS',20175,@app_id,@instance_id );
select @app_id, @instance_id


call log_application_instance_insert2('LogServer','localhost',666)

