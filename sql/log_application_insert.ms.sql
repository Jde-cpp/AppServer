USE [logs]
GO

/****** Object:  StoredProcedure [dbo].[log_application_insert]    Script Date: 4/2/2021 5:26:36 AM ******/
SET ANSI_NULLS ON
GO

drop procedure [log_application_insert]
GO

create procedure [dbo].[log_application_insert]( @name varchar(255), @id int out )
as
	insert into log_applications( name,db_log_level,file_log_level,location )
		values( @name,2,2,null );
	set @id=@@identity;
GO


