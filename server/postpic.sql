
DROP TYPE image CASCADE;
CREATE TYPE image; -- shell type

CREATE FUNCTION image_in ( cstring )
   RETURNS image
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION image_out ( image )
   RETURNS cstring
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;     

CREATE TYPE image (
   input = image_in,
   output = image_out,
   internallength = 40
);

CREATE FUNCTION image_create_from_loid( oid )
   RETURNS image
   AS '$libdir/postpic'
   LANGUAGE C STRICT;

CREATE FUNCTION width ( image )
   RETURNS INT
   AS '$libdir/postpic', 'image_width'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION height ( image )
   RETURNS INT
   AS '$libdir/postpic', 'image_height'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION oid ( image )
   RETURNS INT
   AS '$libdir/postpic', 'image_oid'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION date ( image )
   RETURNS TIMESTAMP
   AS '$libdir/postpic', 'image_date'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION f_number ( image )
   RETURNS FLOAT4
   AS '$libdir/postpic', 'image_f_number'
   LANGUAGE C IMMUTABLE STRICT;
   
CREATE FUNCTION exposure_time ( image )
   RETURNS FLOAT4
   AS '$libdir/postpic', 'image_exposure_time'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION iso ( image )
   RETURNS INT
   AS '$libdir/postpic', 'image_iso'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION postpic_version ( )
   RETURNS cstring
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;
   
CREATE FUNCTION postpic_version_release ( )
   RETURNS INT
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION postpic_version_major ( )
   RETURNS INT
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION postpic_version_minor ( )
   RETURNS INT
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION null_image()
   RETURNS IMAGE AS
$BODY$BEGIN
   RETURN image_create_from_loid(0);
END$BODY$
LANGUAGE 'plpgsql' IMMUTABLE;

CREATE OR REPLACE FUNCTION size ( i image )
	RETURNS INT AS
$BODY$
DECLARE
	w INT;
	h INT;
BEGIN
	SELECT INTO w,h width(i), height(i);
	IF (w > h) THEN
		RETURN w;
	ELSE
		RETURN h;
	END IF;
END
$BODY$
LANGUAGE 'plpgsql' IMMUTABLE;

	