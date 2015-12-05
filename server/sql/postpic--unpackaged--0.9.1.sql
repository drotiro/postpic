/* contrib/postpic/postpic--unpackaged--0.9.1.sql */

-- complain if script is sourced in psql rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION postpic" to load this file. \quit

-- image Type
ALTER EXTENSION postpic ADD TYPE image;
ALTER EXTENSION postpic ADD FUNCTION image_in(cstring);
ALTER EXTENSION postpic ADD FUNCTION image_out(image);
ALTER EXTENSION postpic ADD FUNCTION image_send(image);
ALTER EXTENSION postpic ADD FUNCTION image_recv(internal);

-- colorspace Type
ALTER EXTENSION postpic ADD TYPE colorspace;

-- color Type
ALTER EXTENSION postpic ADD TYPE color;
ALTER EXTENSION postpic ADD FUNCTION color_in(cstring);
ALTER EXTENSION postpic ADD FUNCTION color_out(color);

-- Create image functions
ALTER EXTENSION postpic ADD FUNCTION image_new(INT, INT, color);
ALTER EXTENSION postpic ADD FUNCTION image_from_large_object(oid);
ALTER EXTENSION postpic ADD FUNCTION image_from_bytea(bytea);

-- Image metadata functions
ALTER EXTENSION postpic ADD FUNCTION width(image);
ALTER EXTENSION postpic ADD FUNCTION height(image);
ALTER EXTENSION postpic ADD FUNCTION date(image);
ALTER EXTENSION postpic ADD FUNCTION f_number(image);
ALTER EXTENSION postpic ADD FUNCTION exposure_time(image);
ALTER EXTENSION postpic ADD FUNCTION iso(image);
ALTER EXTENSION postpic ADD FUNCTION focal_length(image);
ALTER EXTENSION postpic ADD FUNCTION colorspace(image);
ALTER EXTENSION postpic ADD FUNCTION size(image);

-- Image manipulation functions
ALTER EXTENSION postpic ADD FUNCTION thumbnail(image, INT);
ALTER EXTENSION postpic ADD FUNCTION square(image, INT);
ALTER EXTENSION postpic ADD FUNCTION draw_text(image, VARCHAR);
-- Image, text, x, y
ALTER EXTENSION postpic ADD FUNCTION draw_text(image, VARCHAR, INT, INT);
-- Image, text, x, y, font family, font size
ALTER EXTENSION postpic ADD FUNCTION draw_text(image, VARCHAR, INT, INT, VARCHAR, INT);
-- Image, text, x, y, font family, font size, color
ALTER EXTENSION postpic ADD FUNCTION draw_text(image, VARCHAR, INT, INT, VARCHAR, INT, color);
ALTER EXTENSION postpic ADD FUNCTION draw_rect(image, BOX, color);
ALTER EXTENSION postpic ADD FUNCTION resize(image, INT, INT);
ALTER EXTENSION postpic ADD FUNCTION crop(image, INT, INT, INT, INT);
ALTER EXTENSION postpic ADD FUNCTION rotate(image, FLOAT4);
ALTER EXTENSION postpic ADD FUNCTION rotate_left(image);
ALTER EXTENSION postpic ADD FUNCTION rotate_right(image);
ALTER EXTENSION postpic ADD FUNCTION index (image[], VARCHAR, INT);

-- Postpic information
ALTER EXTENSION postpic ADD FUNCTION postpic_version();
ALTER EXTENSION postpic ADD FUNCTION postpic_version_release();
ALTER EXTENSION postpic ADD FUNCTION postpic_version_major();
ALTER EXTENSION postpic ADD FUNCTION postpic_version_minor();
