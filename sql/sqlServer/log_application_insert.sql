if OBJECT_ID('log_application_insert') is not null
	drop procedure [log_application_insert]
go
create procedure log_application_insert( @name varchar(255), @id int out )
as
	set nocount on;
	insert into log_applications( name,db_log_level,file_log_level,location )
		values( @name,2,2,null );
	set @id=@@identity;
GO