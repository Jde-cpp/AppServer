﻿use logs;
drop procedure log_message_insert_out;
drop procedure log_message_insert;
drop procedure log_message_insert1;
drop procedure log_message_insert2;
drop procedure log_message_insert3;
drop procedure log_message_insert4;
drop procedure log_message_insert5;

select FROM_UNIXTIME(1563617758)

select ADDDATE(FROM_UNIXTIME(1389422614485000/1000000), INTERVAL mod(1389422614485000,1000000) MICROSECOND);

drop table deletme
create table deletme
(
	timex datetime not null
);
delete from deletme
#insert into deletme values( CONVERT_TZ(ADDDATE(FROM_UNIXTIME(1563617631958551/1000000), INTERVAL mod(1563617631958551,1000000) MICROSECOND), '+00:00', @@session.time_zone) );
insert into deletme values( ADDDATE(FROM_UNIXTIME(1563617631958551/1000000), INTERVAL mod(1563617631958551,1000000) MICROSECOND) );
select * from deletme

SELECT timex local_time, CONVERT_TZ(timex, @@session.time_zone, '+00:00') AS `utc_datetime` 
FROM deletme

select * from log_applications
update log_applications set file_log_level=1

SET time_zone = "+00:00"; 
select time, CONVERT_TZ(time, @@session.time_zone, '+00:00') from logs order by time desc limit 1;

SET time_zone = "+01:00"; 
select time, CONVERT_TZ(time, @@session.time_zone, '+00:00') from logs order by time desc limit 1;

SET time_zone = "-04:00"; 
select time, CONVERT_TZ(time, @@session.time_zone, '+00:00') from logs order by time desc limit 1;


drop procedure log_message_insert_out;
DELIMITER $$
CREATE PROCEDURE log_message_insert_out( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time datetime,  _userId int unsigned, out _id int unsigned )
begin
	INSERT INTO logs(application_id,application_instance_id,file_id,function_id,line_number,message_id,severity,thread_id,time,user_id)
	VALUES( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, CONVERT_TZ(time, '+00:00', @@session.time_zone), _userId );

	select LAST_INSERT_ID() into _id;
	if( mod(_id,10000)=0 ) then
		if( _id>1000000 ) then
			delete from log_variables where log_id<_id-1000000;
			delete from log_logs where id<_id-1000000;
			delete from log_files where id not in (select file_id from logs);
			delete from log_functions where id not in (select function_id from logs);
			delete from log_messages where id not in (select message_id from logs);
		end if;
	end if;
	select _id;
end$$
DELIMITER ;


DELIMITER $$
CREATE PROCEDURE log_message_insert( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time datetime,  _userId int unsigned )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time,  _userId, _id );
end$$
DELIMITER ;

DELIMITER $$
CREATE PROCEDURE log_message_insert1( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time datetime,  _userId int unsigned, _variable0 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
end$$
DELIMITER ;

DELIMITER $$
CREATE PROCEDURE log_message_insert2( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time datetime,  _userId int unsigned, _variable0 varchar(4096), _variable1 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
	insert into log_variables values(_id,1,_variable1);
end$$
DELIMITER ;

#drop procedure log_message_insert3;
DELIMITER $$
CREATE PROCEDURE log_message_insert3( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time datetime,  _userId int unsigned, _variable0 varchar(4096), _variable1 varchar(4096), _variable2 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
	insert into log_variables values(_id,1,_variable1);
	insert into log_variables values(_id,2,_variable2);
end$$
DELIMITER ;

DELIMITER $$
CREATE PROCEDURE log_message_insert4( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time datetime,  _userId int unsigned, _variable0 varchar(4096), _variable1 varchar(4096), _variable2 varchar(4096), _variable3 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
	insert into log_variables values(_id,1,_variable1);
	insert into log_variables values(_id,2,_variable2);
	insert into log_variables values(_id,3,_variable3);
end$$
DELIMITER ;

DELIMITER $$
CREATE PROCEDURE log_message_insert5( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time datetime,  _userId int unsigned, _variable0 varchar(4096), _variable1 varchar(4096), _variable2 varchar(4096), _variable3 varchar(4096), _variable4 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
	insert into log_variables values(_id,1,_variable1);
	insert into log_variables values(_id,2,_variable2);
	insert into log_variables values(_id,3,_variable3);
	insert into log_variables values(_id,4,_variable4);
end$$
DELIMITER ;


