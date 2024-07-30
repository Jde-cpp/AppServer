drop procedure if exists um_user_insert_login;
go

#DELIMITER $$
create procedure um_user_insert_login( modulus varchar(1024), exponent int unsigned, provider_id int unsigned, name varchar(255), target varchar(255), description varchar(2047) )
begin
	CALL um_entity_insert(name, 0, target, description, false, provider_id);
	SET entity_id = LAST_INSERT_ID();

	insert into um_users( entity_id, modulus, exponent ) values( entity_id, modulus, exponent );
	SELECT entity_id;
end
#$$
#DELIMITER ;
#call um_user_insert_login( 'a', 1, null );