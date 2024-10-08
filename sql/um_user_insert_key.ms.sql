create or alter procedure um_user_insert_key( @modulus varchar(1024), @exponent int, @provider_id int, @name varchar(255), @target varchar(255), @description varchar(2047) ) as
begin
	declare @entity_id int;
	exec um_entity_insert @name, @target, @provider_id, 0, @description, false;
	set @entity_id = @@IDENTITY;

	insert into um_users( entity_id, modulus, exponent ) values( @entity_id, @modulus, @exponent );
	select @entity_id;
end