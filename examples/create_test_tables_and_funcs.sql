--DROP TABLE images;
CREATE TABLE images (
		iid		SERIAL NOT NULL,
		the_img	IMAGE,
		name	VARCHAR(64),
		CONSTRAINT pk_images PRIMARY KEY (iid)
);

--DROP TABLE albums;
CREATE TABLE albums (
	aid		SERIAL NOT NULL,
	name	varchar(128) not null,
	description	varchar(1024),
	created	timestamp not null default now(),
	CONSTRAINT pk_albums PRIMARY KEY (aid)
);

--DROP TABLE album_images;
CREATE TABLE album_images (
	aid		INTEGER NOT NULL,
	iid		INTEGER NOT NULL,
	CONSTRAINT pk_album_images PRIMARY KEY (aid, iid)
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

-- A sample callback for postpic_import and related funcs
CREATE OR REPLACE FUNCTION basename(ipath varchar)
	RETURNS	VARCHAR	AS
$BODY$
DECLARE
	pos INTEGER;
	path VARCHAR;
BEGIN
	path := ipath;
	SELECT INTO pos position('/' IN path);
	WHILE (pos > 0) LOOP
		path := substr (path, pos + 1);		
		SELECT INTO pos position('/' IN path);
	END LOOP;
	RETURN path;
END
$BODY$
LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION image_add_to_album(ii integer, aname varchar)
	RETURNS VOID AS
$BODY$
DECLARE
	ai	INTEGER;
BEGIN
	SELECT INTO ai aid from albums WHERE name=aname;
	INSERT INTO album_images (aid, iid) values (ai, ii);
EXCEPTION
	WHEN OTHERS THEN RETURN;
END
$BODY$
LANGUAGE 'plpgsql';


CREATE OR REPLACE FUNCTION postpic_import_callback(i image, ipath varchar, usrdata varchar)
  RETURNS INTEGER AS
$BODY$
DECLARE
	ii INTEGER;
BEGIN
	INSERT INTO images (the_img, name) values (i, basename(ipath));
	IF (usrdata IS NOT NULL) THEN
		SELECT INTO ii last_value from images_iid_seq;
		PERFORM image_add_to_album(ii, usrdata);
	END IF;
	RETURN 0;
EXCEPTION
	WHEN OTHERS THEN 
	RETURN 1;
END
$BODY$
LANGUAGE 'plpgsql';

-- Helpful funcs

CREATE OR REPLACE FUNCTION images_by_month(OUT year DOUBLE PRECISION, OUT month DOUBLE PRECISION, OUT photos BIGINT) 
RETURNS SETOF RECORD AS $$
	SELECT date_part('year', date(the_img)) AS year, date_part('month', date(the_img)) AS month, count(*) AS photos 
	FROM images 
	GROUP BY year, month 
	ORDER BY year, month
$$ LANGUAGE SQL STABLE;

CREATE OR REPLACE FUNCTION album_id( alb varchar )
RETURNS INTEGER AS
$BODY$
DECLARE
	ai INTEGER;
BEGIN
	SELECT INTO ai aid FROM albums WHERE name=alb;
	RETURN ai;
END;
$BODY$
language 'plpgsql' STABLE;

CREATE OR REPLACE FUNCTION in_album( ii integer, ai integer )
	RETURNS BOOLEAN AS
$BODY$
DECLARE
	cnt INTEGER;
BEGIN
	SELECT INTO cnt count(*) FROM album_images WHERE aid=ai AND iid=ii;
	IF (cnt = 1) THEN
		RETURN TRUE;
	ELSE
		RETURN FALSE;
	END IF;
END
$BODY$
language 'plpgsql' STABLE;


-- To test server-side import
CREATE OR REPLACE FUNCTION test_insert ( ipath varchar, iname varchar )
   RETURNS VOID AS
$BODY$DECLARE
   loid OID;
BEGIN
      SELECT INTO loid lo_import(ipath);
      INSERT INTO images (the_img, name) values(image_create_from_loid(loid), iname);
END$BODY$
LANGUAGE 'plpgsql';

