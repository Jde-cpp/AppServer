drop procedure if exists um_user_insert_key;
go

#DELIMITER $$
create procedure um_user_insert_key( modulus varchar(1024), exponent int unsigned, provider_id int unsigned, name varchar(255), target varchar(255), description varchar(2047) )
begin
	declare entity_id int unsigned;
	call um_entity_insert(name, 0, target, description, false, provider_id);
	set entity_id = LAST_INSERT_ID();

	insert into um_users( entity_id, modulus, exponent ) values( entity_id, modulus, exponent );
	select entity_id;
end
#$$
#DELIMITER ;