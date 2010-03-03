/*********************************************************************
 PostPic - An image-enabling extension for PostgreSql
 (C) Copyright 2010 Domenico Rotiroti
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as
 published by the Free Software Foundation, version 3 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 A copy of the GNU Lesser General Public License is included in the
 source distribution of this software.

*********************************************************************/

/* PG-related includes */
#include <postgres.h>
#include <fmgr.h>
/* -> dates & co */
#include <utils/datetime.h>
#include <pgtime.h>
/* -> large objects */
#include <storage/large_object.h>
#include <libpq/libpq-fs.h>
#include <storage/fd.h>
/* --- */
#include <utils/elog.h>
#include <utils/builtins.h>
/* GM-related includes */
#include <magick/api.h>
/* Others */
#include <stdio.h>

PG_MODULE_MAGIC;

/* 
 * The datatype we use internally for storing
 * image information
 */
typedef struct {
 Oid		imgdata;
 Timestamp	date;
 int4	width;
 int4	height;
 int4	cspace;
 int4	iso;
 float4	f_number;
 float4 exposure_t;
 float4 focal_l;
} PPImage;

/* Some constant */
#define BUFSIZE		8192
#define INTLEN		20
#define DATELEN		20
#define VERLEN		128
#define ATTR_TIME	"EXIF:DateTimeOriginal"
#define ATTR_EXPT	"EXIF:ExposureTime"
#define ATTR_FNUM	"EXIF:FNumber"
#define ATTR_ISO	"EXIF:ISOSpeedRatings"
#define	PP_VERSION_RELEASE	0
#define	PP_VERSION_MAJOR	9
#define	PP_VERSION_MINOR	0


/*
 * Mandatory and factory methods 
 */
Datum	image_in(PG_FUNCTION_ARGS);
Datum	image_out(PG_FUNCTION_ARGS);
Datum	image_create_from_loid(PG_FUNCTION_ARGS);

/*
 * A bit of info about ourselves
 */
Datum	postpic_version(PG_FUNCTION_ARGS);
Datum	postpic_version_release(PG_FUNCTION_ARGS);
Datum	postpic_version_major(PG_FUNCTION_ARGS);
Datum	postpic_version_minor(PG_FUNCTION_ARGS);

/*
 * Property getters and operations
 */
Datum	image_width(PG_FUNCTION_ARGS);
Datum	image_height(PG_FUNCTION_ARGS);
Datum	image_oid(PG_FUNCTION_ARGS);
Datum	image_date(PG_FUNCTION_ARGS);
Datum	image_f_number(PG_FUNCTION_ARGS);
Datum	image_exposure_time(PG_FUNCTION_ARGS);
Datum	image_iso(PG_FUNCTION_ARGS);

/*
 * Image processing functions
 */
Datum	image_thumbnail(PG_FUNCTION_ARGS);
Datum	image_square(PG_FUNCTION_ARGS);

/*
 * Internal and GraphicsMagick's
 */
void		pp_init_image(PPImage * img, Image * gimg);
Timestamp	pp_str2timestamp(const char * date);
char *		pp_timestamp2str(Timestamp ts);
int			pp_substr2int(char * str, int off, int len);
float4		pp_parse_float(const char * str);
int			pp_parse_int(char * str);
Image *		gm_image_from_lob(Oid loid);
char *		gm_image_getattr(Image * img, const char * attr);
void *		gm_image_to_blob(Image * timg, size_t * blen, ExceptionInfo * ex);
void		gm_image_destroy(Image *);
void *		lo_readblob(Oid loid, int * len);
int			lo_size(LargeObjectDesc * lod);


PG_FUNCTION_INFO_V1(image_in);
Datum       image_in(PG_FUNCTION_ARGS)
{
	char * oidstr = PG_GETARG_CSTRING(0);
	Oid loid;
	PPImage * img = (PPImage *) palloc(sizeof(PPImage));
	Image * gimg;
	
	sscanf(oidstr, "%d", &loid);
	gimg = gm_image_from_lob(loid);
	pp_init_image(img, gimg);
	img->imgdata = loid;
	gm_image_destroy(gimg);
	PG_RETURN_POINTER(img);
}

PG_FUNCTION_INFO_V1(image_out);
Datum       image_out(PG_FUNCTION_ARGS)
{
	PPImage * img = (PPImage *) PG_GETARG_POINTER(0);
	char * out = palloc(8*INTLEN+DATELEN);
	sprintf(out, "%d|%s|%d|%d|%f|%f|%d", img->imgdata, 
		pp_timestamp2str(img->date), img->width, img->height,
		img->f_number, img->exposure_t, img->iso);
	PG_RETURN_CSTRING(out);	
}

PG_FUNCTION_INFO_V1(image_create_from_loid);
Datum		image_create_from_loid(PG_FUNCTION_ARGS)
{
	Oid loid = PG_GETARG_OID(0);
	PPImage * img = palloc(sizeof(PPImage));
	Image * gimg = gm_image_from_lob(loid);
	
	pp_init_image(img, gimg);
	img->imgdata = loid;
	gm_image_destroy(gimg);
	PG_RETURN_POINTER(img);
}

PG_FUNCTION_INFO_V1(image_thumbnail);
Datum   image_thumbnail(PG_FUNCTION_ARGS)
{
	ExceptionInfo ex;
	void * blob;
	int4 size, sx, sy;
	size_t blen;
	PPImage * img;
	bytea * result;
	Image * gimg, * timg;
	
	img = (PPImage *) PG_GETARG_POINTER(0);
	size = PG_GETARG_INT32(1);
	gimg = gm_image_from_lob(img->imgdata);
	if(img->width >= img->height) {
		sx = size;
		sy = img->height * size / img->width;
	} else {
		sy = size;
		sx = img->width * size / img->height;
	}
	GetExceptionInfo(&ex);
	timg = ThumbnailImage(gimg, sx, sy, &ex);
	blob = gm_image_to_blob(timg, &blen, &ex);
	
    result = (bytea*) palloc(VARHDRSZ+blen);
    SET_VARSIZE(result, VARHDRSZ+blen);
    memcpy(VARDATA(result), blob, blen);
            
	free(blob);
	gm_image_destroy(gimg);
	gm_image_destroy(timg);
	DestroyExceptionInfo(&ex);
	PG_RETURN_BYTEA_P(result);	
}

PG_FUNCTION_INFO_V1(image_square);
Datum   image_square(PG_FUNCTION_ARGS)
{
	ExceptionInfo ex;
	RectangleInfo ri;
	void * blob;
	int4 size, sx, sy;
	size_t blen;
	PPImage * img;
	bytea * result;
	Image * gimg, * timg, * simg;
	
	img = (PPImage *) PG_GETARG_POINTER(0);
	size = PG_GETARG_INT32(1);
	gimg = gm_image_from_lob(img->imgdata);

	ri.x = ri.y = 0;
	ri.width = ri.height = size;
	if(img->width < img->height) {
		sx = size;
		sy = img->height * size / img->width;
		ri.y = (sy-sx)/2;
	} else {
		sy = size;
		sx = img->width * size / img->height;
		ri.x = (sx-sy)/2;
	}
	GetExceptionInfo(&ex);
	timg = ThumbnailImage(gimg, sx, sy, &ex);
	simg = CropImage(timg, &ri, &ex);
	blob = gm_image_to_blob(simg, &blen, &ex);
	
    result = (bytea*) palloc(VARHDRSZ+blen);
    SET_VARSIZE(result, VARHDRSZ+blen);
    memcpy(VARDATA(result), blob, blen);
            
	free(blob);
	gm_image_destroy(gimg);
	gm_image_destroy(timg);
	gm_image_destroy(simg);
	DestroyExceptionInfo(&ex);
	PG_RETURN_BYTEA_P(result);	
}

void * gm_image_to_blob(Image * timg, size_t * blen, ExceptionInfo * ex)
{
	ImageInfo *iinfo;
	void * blob;

	iinfo = CloneImageInfo(NULL);
	strcpy(iinfo->magick, "JPEG");
	blob = ImageToBlob(iinfo, timg, blen, ex);
	DestroyImageInfo(iinfo);
	
	return blob;
}

PG_FUNCTION_INFO_V1(postpic_version);
Datum		postpic_version(PG_FUNCTION_ARGS)
{
	char * version = palloc(DATELEN);
	sprintf(version, "PostPic version %d.%d.%d", PP_VERSION_RELEASE,
		PP_VERSION_MAJOR, PP_VERSION_MINOR);
	PG_RETURN_CSTRING( version ); 
}

PG_FUNCTION_INFO_V1(postpic_version_release);
Datum		postpic_version_release(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32( PP_VERSION_RELEASE );
}

PG_FUNCTION_INFO_V1(postpic_version_major);
Datum		postpic_version_major(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32( PP_VERSION_MAJOR );
}

PG_FUNCTION_INFO_V1(postpic_version_minor);
Datum		postpic_version_minor(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32( PP_VERSION_MINOR );
}

PG_FUNCTION_INFO_V1(image_width);
Datum		image_width(PG_FUNCTION_ARGS)
{
	PPImage * img = (PPImage *) PG_GETARG_POINTER(0);
	PG_RETURN_INT32(img->width);
}

PG_FUNCTION_INFO_V1(image_height);
Datum		image_height(PG_FUNCTION_ARGS)
{
	PPImage * img = (PPImage *) PG_GETARG_POINTER(0);
	PG_RETURN_INT32(img->height);
}

PG_FUNCTION_INFO_V1(image_oid);
Datum		image_oid(PG_FUNCTION_ARGS)
{
	PPImage * img = (PPImage *) PG_GETARG_POINTER(0);
	PG_RETURN_OID(img->imgdata);
}

PG_FUNCTION_INFO_V1(image_date);
Datum		image_date(PG_FUNCTION_ARGS)
{
	PPImage * img = (PPImage *) PG_GETARG_POINTER(0);
	if(TIMESTAMP_IS_NOBEGIN(img->date)) PG_RETURN_NULL();
	PG_RETURN_TIMESTAMP(img->date);
}

PG_FUNCTION_INFO_V1(image_f_number);
Datum		image_f_number(PG_FUNCTION_ARGS)
{
    PPImage * img = (PPImage *) PG_GETARG_POINTER(0);
    if(img->f_number > 0) {
    	PG_RETURN_FLOAT4(img->f_number);
	}
	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(image_exposure_time);
Datum		image_exposure_time(PG_FUNCTION_ARGS)
{
    PPImage * img = (PPImage *) PG_GETARG_POINTER(0);
    if(img->exposure_t > 0) {
        PG_RETURN_FLOAT4(img->exposure_t);
    }
    PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(image_iso);
Datum		image_iso(PG_FUNCTION_ARGS)
{
    PPImage * img = (PPImage *) PG_GETARG_POINTER(0);
    if(img->iso > 0) {
        PG_RETURN_INT32(img->iso);
    }
    PG_RETURN_NULL();
}

Image *		gm_image_from_lob(Oid loid)
{
	ExceptionInfo ex;
	ImageInfo * iinfo;
	Image * res;
	int blen;
	void * blob;

	if(!OidIsValid(loid)) return NULL;
	//elog(NOTICE, "Reading image from lob %d", loid);
	GetExceptionInfo(&ex);
	iinfo = CloneImageInfo(NULL);
	blob = lo_readblob(loid, &blen);
	res = BlobToImage(iinfo, blob, blen,  &ex); 
	DestroyImageInfo(iinfo);
	DestroyExceptionInfo(&ex);

	return res;
}

void	gm_image_destroy(Image * gimg)
{
	if(gimg) DestroyImage(gimg);
}

char *	gm_image_getattr(Image * img, const char * attr)
{
	const ImageAttribute * attrs = GetImageAttribute(img, attr);
	if(!attrs) return NULL;
	return pstrdup(attrs->value);
}

int		pp_substr2int(char * str, int off, int len)
{
	str[off+len] = 0;
	return atoi(str+off);
}

Timestamp	pp_str2timestamp(const char * edate)
{
	struct pg_tm tm;
	Timestamp res;
	char * date;
	
	if(!edate || strlen(edate)<19) {
    	//elog(WARNING, "could not parse date %s", edate);
	    TIMESTAMP_NOBEGIN(res);
	    return res;
	}
	date = pstrdup(edate);
	tm.tm_year = pp_substr2int(date, 0, 4);
	tm.tm_mon =  pp_substr2int(date, 5, 2);
	tm.tm_mday = pp_substr2int(date, 8, 2);
	tm.tm_hour = pp_substr2int(date, 11, 2);
	tm.tm_min = pp_substr2int(date, 14, 2);
	tm.tm_sec = pp_substr2int(date, 17, 2);
	tm2timestamp(&tm, 0, NULL, &res);
	return res;
}

char*		pp_timestamp2str(Timestamp ts)
{
	/* postgres tax */
	int tzp;
	fsec_t fsec;
	char *tzn;
	/* end tax */
	struct pg_tm tm;
	char * res; 
	
	if(TIMESTAMP_IS_NOBEGIN(ts)) {
		return pstrdup("");
	}
	res = palloc(DATELEN);
	timestamp2tm(ts, &tzp, &tm, &fsec , &tzn, NULL);
	sprintf(res, "%d-%.2d-%.2d %.2d:%.2d:%.2d", tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	return res;
}

float4		pp_parse_float(const char * str)
{
    float s=-1,g=1;
    if(!str || !str[0]) return -1;
    if(index(str, '/')==NULL) return strtod(str,NULL);
    sscanf(str, "%f/%f", &s, &g);
    return s/g;
}

int		pp_parse_int(char * str)
{
	if(!str || !isdigit(str[0])) return -1;
	return pg_atoi(str, 4, 0);
}

void	pp_init_image(PPImage * img, Image * gimg)
{
	char * attr;

	memset(img, 0, sizeof(img));
	if(!gimg) return;

	//Data we have in Image
	img->width = gimg->columns;
	img->height = gimg->rows;
	
	//Data read from attributes
	attr = gm_image_getattr(gimg, ATTR_TIME);
	img->date = pp_str2timestamp(attr);
    attr = gm_image_getattr(gimg, ATTR_FNUM);
   	img->f_number = pp_parse_float(attr);
	attr = gm_image_getattr(gimg, ATTR_EXPT);
   	img->exposure_t = pp_parse_float(attr);
   	attr = gm_image_getattr(gimg, ATTR_ISO);
   	img->iso = pp_parse_int(attr);
}


int		lo_size(LargeObjectDesc * lod)
{
	inv_seek(lod, 0, SEEK_END);
	return inv_tell(lod);
}

void *	lo_readblob(Oid loid, int * blen)
{
	void * buf;
	int size;
	int tmp;
	
	LargeObjectDesc * lod = inv_open(loid, INV_READ, CurrentMemoryContext);
	size = lo_size(lod);
	inv_seek(lod, 0, SEEK_SET);
	*blen = size;
	buf = palloc(size);
    if(!buf) {
        ereport(ERROR,
        	(errcode(ERRCODE_UNDEFINED_OBJECT),
			 errmsg("error reading from large object: %d", loid)));
    }
	tmp = inv_read(lod, buf, size);
	inv_close(lod);
	
	return buf;
}

/*
 * The easyest way to check...
 */
/*
void _PG_init()
{
	elog(NOTICE, "type image's size is: %d", sizeof(PPImage));
}
*/
