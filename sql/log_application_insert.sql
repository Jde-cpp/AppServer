drop procedure if exists log_application_insert;

DELIMITER $$;
CREATE PROCEDURE log_application_insert( _name varchar(255), out _id int unsigned )
begin
        insert into log_applications(name, db_log_level, file_log_level) values(_name, 2, 2);
        select LAST_INSERT_ID() into _id;
end;
$$
DELIMITER ;
