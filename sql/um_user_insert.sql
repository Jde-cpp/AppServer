drop procedure if exists um_user_insert;
go

#DELIMITER $$
create procedure um_user_insert( _entity_id int unsigned, _login_name varchar(255), _password varchar(2047) )
begin
	insert into um_users( entity_id, login_name, password ) values( _entity_id, _login_name, _password );
	SELECT _entity_id;
end
#$$
#DELIMITER ;
