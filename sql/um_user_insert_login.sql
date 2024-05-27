drop procedure if exists um_user_insert_login;
go

#DELIMITER $$
create procedure um_user_insert_login( _login_name varchar(255),_provider_id smallint,_provider_target varchar(255) )
begin
	declare provider_id int unsigned; declare entity_id int unsigned; declare target varchar(255); declare provider_name varchar(255);
	if( _provider_target is not null ) then
		set provider_id = (select id from um_providers where target = _provider_target);
        set provider_name =  _provider_target;
	else
		set provider_id = _provider_id;
        if( provider_id is not null ) then
			set provider_name = (select name from um_provider_types where id=provider_id );
		end if;
	end if;
    if( provider_name is not null ) then
		set target = CONCAT( provider_name, '-', _login_name );
	else
		set target = _login_name;
	end if;
	CALL um_entity_insert(_login_name, 0, target, null, false, provider_id);
	SET entity_id = LAST_INSERT_ID();

	insert into um_users( entity_id, login_name ) values( entity_id, _login_name );
	SELECT entity_id;
end
#$$
#DELIMITER ;
#call um_user_insert_login( 'a', 1, null );