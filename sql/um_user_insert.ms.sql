create or alter proc um_user_insert( @entity_id int, @login_name varchar(255), @password varchar(2047), @modulus varchar(1024), @exponent int ) as
begin
	insert into um_users( entity_id, login_name, password, modulus, exponent ) values( @entity_id, @login_name, @password, @modulus, @exponent  );
	select @entity_id;
end
