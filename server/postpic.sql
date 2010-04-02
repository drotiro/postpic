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

CREATE FUNCTION image_send ( image )
	RETURNS bytea
	AS '$libdir/postpic'
	LANGUAGE C IMMUTABLE STRICT;
	
CREATE FUNCTION image_recv ( internal )
	RETURNS image
	AS '$libdir/postpic'
	LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE image (
   input = image_in,
   output = image_out,
   receive = image_recv,
   send = image_send,
   internallength = variable,
   storage = EXTERNAL
);

-- Support type 'colorspace'
CREATE TYPE colorspace as ENUM (
	'Unknown',
	'RGB',
	'RGBA',
	'Gray',
	'sRGB',
	'CMYK'
);

-- and color
CREATE TYPE color;

CREATE FUNCTION color_in ( cstring )
   RETURNS color
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION color_out ( color )
   RETURNS cstring
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE color (
	input = color_in,
	output = color_out,
	internallength = 8
);

/*
 * Make sure plpgsql is in
 */
CREATE LANGUAGE plpgsql;

CREATE FUNCTION image_new( INT, INT, color )
	RETURNS image
	AS '$libdir/postpic'
	LANGUAGE C STRICT;

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

CREATE FUNCTION focal_length ( image )
   RETURNS FLOAT4
   AS '$libdir/postpic', 'image_focal_length'
   LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION colorspace ( image )
   RETURNS colorspace
   AS '$libdir/postpic', 'image_colorspace'
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

CREATE FUNCTION index (image[], VARCHAR, INT )
	RETURNS image
	AS '$libdir/postpic', 'image_index'
	LANGUAGE C STRICT;

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
