/*﻿
use logs;

drop procedure log_message_insert_out;
drop procedure log_message_insert;
drop procedure log_message_insert1;
drop procedure log_message_insert2;
drop procedure log_message_insert3;
drop procedure log_message_insert4;
drop procedure log_message_insert5;

drop procedure log_application_insert
drop procedure log_application_instance_insert
drop procedure log_application_instance_insert2
drop procedure log_host_insert
drop procedure log_insert

drop table log_variables
drop table logs
drop table log_files
drop table log_functions
drop table log_messages
drop table log_application_instances
drop table log_hosts
drop table log_applications
*/

drop procedure log_message_insert_out;
go
create proc log_message_insert_out( @application_id int, @application_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @time datetime2,  @userId int, @id bigint out )
as
	INSERT INTO logs(application_id,application_instance_id,file_id,function_id,line_number,message_id,severity,thread_id,time,user_id)
	VALUES( @application_id, @application_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @time, @userId );

	set @id=@@IDENTITY;
	if @id%10000=0 begin
		if @id>1000000  begin
			delete from log_variables where log_id<@id-1000000;
			delete from logs where id<@id-1000000;
			delete from log_files where id not in (select file_id from logs);
			delete from log_functions where id not in (select function_id from logs);
			delete from log_messages where id not in (select message_id from logs);
		end
	end
	select @id;
go
drop procedure log_message_insert;
go
create proc log_message_insert( @application_id int, @application_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @time datetime2,  @userId int )
as
	set nocount on
	declare @id int;
	exec log_message_insert_out @application_id, @application_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @time,  @userId, @id;
go
drop procedure log_message_insert1;
go
create proc log_message_insert1( @application_id int, @application_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @time datetime2,  @userId int, @variable0 varchar(4095) )
as
	set nocount on
	declare @id int;
	exec log_message_insert_out @application_id, @application_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @time,  @userId, @id;
	insert into log_variables values(@id,0,@variable0);
go
--declare @id int;
--log_message_insert1(1001,1001,1,377783547,1827296884,8,-384809927,1,888,getutcdate(),5, @id out )

create proc log_message_insert2( @application_id int, @application_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @time datetime2,  @userId int, @variable0 varchar(4095), @variable1 varchar(4095) )
as
	set nocount on
	declare @id int;
	exec log_message_insert_out @application_id, @application_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @time, @userId, @id;
	insert into log_variables values(@id,0,@variable0);
	insert into log_variables values(@id,1,@variable1);
go


drop procedure log_message_insert3;
go
create proc log_message_insert3( @application_id int, @application_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @time datetime2,  @userId int, @variable0 varchar(4095), @variable1 varchar(4095), @variable2 varchar(4095) )
as
	set nocount on
	declare @id int;
	exec log_message_insert_out @application_id, @application_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @time, @userId, @id;
	insert into log_variables values(@id,0,@variable0);
	insert into log_variables values(@id,1,@variable1);
	insert into log_variables values(@id,2,@variable2);
go

drop procedure log_message_insert4;
go
create proc log_message_insert4( @application_id int, @application_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @time datetime2,  @userId int, @variable0 varchar(4095), @variable1 varchar(4095), @variable2 varchar(4095), @variable3 varchar(4095) )
as
	set nocount on
	declare @id int;
	exec log_message_insert_out @application_id, @application_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @time, @userId, @id;
	insert into log_variables values(@id,0,@variable0);
	insert into log_variables values(@id,1,@variable1);
	insert into log_variables values(@id,2,@variable2);
	insert into log_variables values(@id,3,@variable3);
go

drop procedure log_message_insert5;
go
create proc log_message_insert5( @application_id int, @application_instance_id int, @fileId int, @functionId int, @lineNumber smallint, @messageId int, @level tinyint, @threadId bigint, @time datetime2,  @userId int, @variable0 varchar(4095), @variable1 varchar(4095), @variable2 varchar(4095), @variable3 varchar(4095), @variable4 varchar(4095) )
as
	set nocount on
	declare @id int;
	exec log_message_insert_out @application_id, @application_instance_id, @fileId, @functionId, @lineNumber, @messageId, @level, @threadId, @time, @userId, @id;
	insert into log_variables values(@id,0,@variable0);
	insert into log_variables values(@id,1,@variable1);
	insert into log_variables values(@id,2,@variable2);
	insert into log_variables values(@id,3,@variable3);
	insert into log_variables values(@id,4,@variable4);
go

select COUNT(*) from logs

select * from log_functions
521822810
select * from log_messages