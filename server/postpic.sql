/*
 * Definition of our basic type: image
 */
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
   internallength = variable,
   storage = EXTERNAL
);

/*
 * Make sure plpgsql is in
 */
CREATE LANGUAGE plpgsql;

CREATE FUNCTION image_from_large_object( oid )
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

CREATE FUNCTION thumbnail ( image, INT )
	RETURNS image
	AS '$libdir/postpic', 'image_thumbnail'
	LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION square ( image, INT )
    RETURNS image
	AS '$libdir/postpic', 'image_square'
	LANGUAGE C IMMUTABLE STRICT;        

CREATE FUNCTION resize ( image, INT, INT )
	RETURNS image
    AS '$libdir/postpic', 'image_resize'
    LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION crop ( image, INT, INT, INT, INT )
	RETURNS image
	AS '$libdir/postpic', 'image_crop'
	LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION rotate ( image, FLOAT4 )
	RETURNS image
	AS '$libdir/postpic', 'image_rotate'
	LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION rotate_left ( image )
	RETURNS image AS $$
		SELECT rotate($1, -90)
	$$ LANGUAGE SQL IMMUTABLE STRICT;
	
CREATE FUNCTION rotate_right ( image )
	RETURNS image AS $$
		SELECT rotate($1, 90)
	$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE FUNCTION montage_reduce (image[], INT, INT )
	RETURNS image
	AS '$libdir/postpic', 'image_montage_reduce'
	LANGUAGE C STRICT;

/*
CREATE FUNCTION array_append_imgoid( oid[], image ) 
	RETURNS oid[] AS $$
		SELECT array_append ($1, oid($2)::oid)
	$$ LANGUAGE SQL STRICT;

CREATE AGGREGATE index ( image )
(
	sfunc = array_append_imgoid,
	stype = oid[],
	initcond = '{}'
);
*/

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
