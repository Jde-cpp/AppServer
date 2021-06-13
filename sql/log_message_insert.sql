﻿#DELIMITER $$
CREATE PROCEDURE log_message_insert_out( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time char(30),  _userId int unsigned, out _id int unsigned )
begin
	INSERT INTO logs(application_id,application_instance_id,file_id,function_id,line_number,message_id,severity,thread_id,time,user_id)
	VALUES( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, CONVERT_TZ(_time, '+00:00', @@session.time_zone), _userId );

	select LAST_INSERT_ID() into _id;
	if( mod(_id,10000)=0 ) then
		if( _id>1000000 ) then
			delete from log_variables where log_id<_id-1000000;
			delete from logs where id<_id-1000000;
			delete from log_files where id not in (select file_id from logs);
			delete from log_functions where id not in (select function_id from logs);
			delete from log_messages where id not in (select message_id from logs);
		end if;
	end if;
	select _id;
end
#$$
#DELIMITER ;
go
drop procedure if exists log_message_insert;
go
#DELIMITER $$
CREATE PROCEDURE log_message_insert( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time char(30),  _userId int unsigned )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time,  _userId, _id );
end
#$$
#DELIMITER ;
go
drop procedure if exists log_message_insert1;
go
#DELIMITER $$
CREATE PROCEDURE log_message_insert1( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time char(30),  _userId int unsigned, _variable0 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
end
#$$
#DELIMITER ;
go
drop procedure if exists log_message_insert2;
go
#DELIMITER $$
CREATE PROCEDURE log_message_insert2( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time char(30),  _userId int unsigned, _variable0 varchar(4096), _variable1 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
	insert into log_variables values(_id,1,_variable1);
end
#$$
#DELIMITER ;
go
drop procedure if exists log_message_insert3;
go
#DELIMITER $$
CREATE PROCEDURE log_message_insert3( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time char(30),  _userId int unsigned, _variable0 varchar(4096), _variable1 varchar(4096), _variable2 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
	insert into log_variables values(_id,1,_variable1);
	insert into log_variables values(_id,2,_variable2);
end
#$$
#DELIMITER ;
go
drop procedure if exists log_message_insert4;
go
#DELIMITER $$
CREATE PROCEDURE log_message_insert4( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time char(30),  _userId int unsigned, _variable0 varchar(4096), _variable1 varchar(4096), _variable2 varchar(4096), _variable3 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
	insert into log_variables values(_id,1,_variable1);
	insert into log_variables values(_id,2,_variable2);
	insert into log_variables values(_id,3,_variable3);
end
#$$
#DELIMITER ;
go
drop procedure if exists log_message_insert5;
go
#DELIMITER $$
CREATE PROCEDURE log_message_insert5( _application_id int unsigned, _application_instance_id int unsigned, _fileId int unsigned, _functionId int unsigned, _lineNumber smallint unsigned, _messageId int unsigned, _level tinyint, _threadId bigint unsigned, _time char(30),  _userId int unsigned, _variable0 varchar(4096), _variable1 varchar(4096), _variable2 varchar(4096), _variable3 varchar(4096), _variable4 varchar(4096) )
begin
	declare _id int unsigned;
	call log_message_insert_out( _application_id, _application_instance_id, _fileId, _functionId, _lineNumber, _messageId, _level, _threadId, _time, _userId, _id );
	insert into log_variables values(_id,0,_variable0);
	insert into log_variables values(_id,1,_variable1);
	insert into log_variables values(_id,2,_variable2);
	insert into log_variables values(_id,3,_variable3);
	insert into log_variables values(_id,4,_variable4);
end
#$$
#DELIMITER ;