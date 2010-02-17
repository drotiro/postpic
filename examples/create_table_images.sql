DROP TABLE images CASCADE;
CREATE TABLE images (
		iid		SERIAL NOT NULL,
		the_img	IMAGE,
		name	VARCHAR(64),
		CONSTRAINT pk_images PRIMARY KEY (iid)
);


CREATE OR REPLACE FUNCTION on_delete_image() RETURNS TRIGGER AS 
$BODY$BEGIN
	PERFORM lo_unlink(oid(OLD.the_img));
	RETURN OLD;
EXCEPTION
  WHEN OTHERS THEN
    RETURN OLD;
END$BODY$
LANGUAGE 'plpgsql';

CREATE OR REPLACE TRIGGER t_delete_image AFTER DELETE ON images 
	FOR EACH ROW EXECUTE PROCEDURE on_delete_image();

-- For testing purposes
CREATE OR REPLACE FUNCTION test_insert ( ipath varchar, iname varchar )
   RETURNS VOID AS
$BODY$DECLARE
   loid OID;
BEGIN
      SELECT INTO loid lo_import(ipath);
      INSERT INTO images (the_img, name) values(image_create_from_loid(loid), iname);
END$BODY$
LANGUAGE 'plpgsql';

