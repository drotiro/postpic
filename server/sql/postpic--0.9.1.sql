-- Make sure installed through extension mechanism
-- ====================================================

\echo Use "CREATE EXTENSION postpic" to load this file. \quit

-- ====================================================


-- ====================================================
-- Definition of our basic type: image
-- ====================================================

CREATE OR REPLACE TYPE image; -- shell type

CREATE OR REPLACE FUNCTION image_in ( cstring )
   RETURNS image
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION image_out ( image )
   RETURNS cstring
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;     

CREATE OR REPLACE FUNCTION image_send ( image )
	RETURNS bytea
	AS '$libdir/postpic'
	LANGUAGE C IMMUTABLE STRICT;
	
CREATE OR REPLACE FUNCTION image_recv ( internal )
	RETURNS image
	AS '$libdir/postpic'
	LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE TYPE image (
   input = image_in,
   output = image_out,
   receive = image_recv,
   send = image_send,
   internallength = variable,
   storage = EXTERNAL
);

-- ====================================================


-- ====================================================
-- Definition of our basic type: colorspace
-- ====================================================

CREATE OR REPLACE TYPE colorspace as ENUM (
	'Unknown',
	'RGB',
	'RGBA',
	'Gray',
	'sRGB',
	'CMYK'
);

-- ====================================================


-- ====================================================
-- Definition of our basic type: color
-- ====================================================

CREATE OR REPLACE TYPE color; -- shell type

CREATE OR REPLACE FUNCTION color_in ( cstring )
   RETURNS color
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION color_out ( color )
   RETURNS cstring
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE TYPE color (
	input = color_in,
	output = color_out,
	internallength = 8
);

-- ====================================================


-- ====================================================
-- Image Creation Functions
-- ====================================================

CREATE OR REPLACE FUNCTION image_new( INT, INT, color )
	RETURNS image
	AS '$libdir/postpic'
	LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION image_from_large_object( oid )
   RETURNS image
   AS '$libdir/postpic'
   LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION image_from_bytea( bytea )
   RETURNS image
   AS '$libdir/postpic'
   LANGUAGE C STRICT;

-- ====================================================


-- ====================================================
-- Image Metadata Functions
-- ====================================================

CREATE OR REPLACE FUNCTION width ( image )
   RETURNS INT
   AS '$libdir/postpic', 'image_width'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION height ( image )
   RETURNS INT
   AS '$libdir/postpic', 'image_height'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION date ( image )
   RETURNS TIMESTAMP
   AS '$libdir/postpic', 'image_date'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION f_number ( image )
   RETURNS FLOAT4
   AS '$libdir/postpic', 'image_f_number'
   LANGUAGE C IMMUTABLE STRICT;
   
CREATE OR REPLACE FUNCTION exposure_time ( image )
   RETURNS FLOAT4
   AS '$libdir/postpic', 'image_exposure_time'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION iso ( image )
   RETURNS INT
   AS '$libdir/postpic', 'image_iso'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION focal_length ( image )
   RETURNS FLOAT4
   AS '$libdir/postpic', 'image_focal_length'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION colorspace ( image )
   RETURNS colorspace
   AS '$libdir/postpic', 'image_colorspace'
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

-- ====================================================


-- ====================================================
-- Image Manipulation Functions
-- ====================================================

CREATE OR REPLACE FUNCTION thumbnail ( image, INT )
	RETURNS image
	AS '$libdir/postpic', 'image_thumbnail'
	LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION square ( image, INT )
    RETURNS image
	AS '$libdir/postpic', 'image_square'
	LANGUAGE C IMMUTABLE STRICT;        

CREATE OR REPLACE FUNCTION draw_text ( image, VARCHAR )
    RETURNS image
    AS '$libdir/postpic', 'image_draw_text'
    LANGUAGE C IMMUTABLE STRICT;

-- Image, text, x, y
CREATE OR REPLACE FUNCTION draw_text ( image, VARCHAR, INT, INT )
    RETURNS image
    AS '$libdir/postpic', 'image_draw_text'
    LANGUAGE C IMMUTABLE STRICT;

-- Image, text, x, y, font family, font size
CREATE OR REPLACE FUNCTION draw_text ( image, VARCHAR, INT, INT, VARCHAR, INT )
    RETURNS image
    AS '$libdir/postpic', 'image_draw_text'
    LANGUAGE C IMMUTABLE STRICT;

-- Image, text, x, y, font family, font size, color
CREATE OR REPLACE FUNCTION draw_text ( image, VARCHAR, INT, INT, VARCHAR, INT, color )
    RETURNS image
    AS '$libdir/postpic', 'image_draw_text'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION draw_rect ( image, BOX, color )
    RETURNS image
    AS '$libdir/postpic', 'image_draw_rect'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION resize ( image, INT, INT )
	RETURNS image
    AS '$libdir/postpic', 'image_resize'
    LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION crop ( image, INT, INT, INT, INT )
	RETURNS image
	AS '$libdir/postpic', 'image_crop'
	LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION rotate ( image, FLOAT4 )
	RETURNS image
	AS '$libdir/postpic', 'image_rotate'
	LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION rotate_left ( image )
	RETURNS image AS $$
		SELECT rotate($1, -90)
	$$ LANGUAGE SQL IMMUTABLE STRICT;
	
CREATE OR REPLACE FUNCTION rotate_right ( image )
	RETURNS image AS $$
		SELECT rotate($1, 90)
	$$ LANGUAGE SQL IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION index (image[], VARCHAR, INT )
	RETURNS image
	AS '$libdir/postpic', 'image_index'
	LANGUAGE C STRICT;

-- ====================================================


-- ====================================================
-- PostPic Metadata functions
-- ====================================================

CREATE OR REPLACE FUNCTION postpic_version ( )
   RETURNS cstring
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;
   
CREATE OR REPLACE FUNCTION postpic_version_release ( )
   RETURNS INT
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION postpic_version_major ( )
   RETURNS INT
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION postpic_version_minor ( )
   RETURNS INT
   AS '$libdir/postpic'
   LANGUAGE C IMMUTABLE STRICT;

-- ====================================================
