if OBJECT_ID('log_application_instance_insert') is not null
	drop procedure log_application_instance_insert;
go
CREATE PROCEDURE log_application_instance_insert( @application_name varchar(255), @host_name varchar(255), @process_id bigint )
as
	set nocount on;
	declare @host_id int; 
	declare @application_id int; 
	declare @instance_id int;
	declare @db_log_level int;
	declare @file_log_level int;
	select @application_id=id, @db_log_level=db_log_level, @file_log_level=file_log_level  from log_applications where name=@application_name;
	if( @application_id is null )
		exec log_application_insert @application_name, @application_id out;

	select @host_id=id from log_hosts where name=@host_name;
	if @host_id is null
	begin
		insert into log_hosts( name ) values( @host_name );
		set @host_id = @@identity;
	end
	insert into log_application_instances(application_id,end_time,host_id,process_id,start) values( @application_id,null,@host_id,@process_id,getutcdate() );
	set @instance_id = @@IDENTITY;
   select @application_id, @instance_id, @db_log_level, @file_log_level;
go