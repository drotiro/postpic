/*******************************
 PostPic - An image-enabling
 extension for PostgreSql
 
 Author:	Domenico Rotiroti
 License:	LGPLv3
*******************************/

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
 //int4	iso;
} PPImage;

/* Some constant */
#define BUFSIZE		8192
#define OIDLEN		20
#define DATELEN		20
#define VERLEN		128
#define ATTR_TIME	"EXIF:DateTimeOriginal"
#define	PP_VERSION_RELEASE	1
#define	PP_VERSION_MAJOR	0
#define	PP_VERSION_MINOR	0


/*
 * Mandatory and factory methods 
 */
Datum		image_in(PG_FUNCTION_ARGS);
Datum		image_out(PG_FUNCTION_ARGS);
Datum		image_create_from_loid(PG_FUNCTION_ARGS);

/*
 * A bit of info about ourselves
 */
Datum		postpic_version(PG_FUNCTION_ARGS);
Datum		postpic_version_release(PG_FUNCTION_ARGS);
Datum		postpic_version_major(PG_FUNCTION_ARGS);
Datum		postpic_version_minor(PG_FUNCTION_ARGS);

/*
 * Property getters and operations
 */
Datum		image_width(PG_FUNCTION_ARGS);
Datum		image_height(PG_FUNCTION_ARGS);
Datum		image_oid(PG_FUNCTION_ARGS);
Datum		image_date(PG_FUNCTION_ARGS);

/*
 * Internal and GraphicsMagick's
 */
void		pp_init_image(PPImage * img, Image * gimg);
Timestamp	pp_str2timestamp(const char * date);
char*		pp_timestamp2str(Timestamp ts);
int			pp_substr2int(char * str, int off, int len);
Image *		gm_image_from_lob(Oid loid);
char *		gm_image_getattr(Image * img, const char * attr);
void		gm_image_destroy(Image *);
FILE *		lo_copy_out(Oid loid);


PG_FUNCTION_INFO_V1(image_in);
Datum       image_in(PG_FUNCTION_ARGS)
{
	char * oidstr = PG_GETARG_CSTRING(0);
	Oid loid;
	PPImage * img = (PPImage *) palloc(sizeof(img));
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
	char * out = palloc(3*OIDLEN+DATELEN);
	sprintf(out, "%d|%s|%d|%d", img->imgdata, 
		pp_timestamp2str(img->date), img->width, img->height);
	PG_RETURN_CSTRING(out);	
}

PG_FUNCTION_INFO_V1(image_create_from_loid);
Datum		image_create_from_loid(PG_FUNCTION_ARGS)
{
	Oid loid = PG_GETARG_OID(0);
	PPImage * img = palloc(sizeof(img));
	Image * gimg = gm_image_from_lob(loid);
	
	pp_init_image(img, gimg);
	img->imgdata = loid;
	gm_image_destroy(gimg);
	PG_RETURN_POINTER(img);
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
	PG_RETURN_TIMESTAMP(img->date);
}

Image *		gm_image_from_lob(Oid loid)
{
	ExceptionInfo ex;
	ImageInfo * iinfo;
	Image * res;
	FILE * tmpf;

	if(!OidIsValid(loid)) return NULL;
	//elog(NOTICE, "Reading image from lob %d", loid);
	GetExceptionInfo(&ex);
	iinfo = CloneImageInfo(NULL);
	tmpf = lo_copy_out(loid);
	iinfo->file = tmpf;
	res = ReadImage(iinfo, &ex);
	fclose(tmpf);
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
	
	if(strlen(edate)<19) {
	    elog(WARNING, "could not parse date %s", edate);
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
	//struct pg_tz atz;
	int tzp;
	fsec_t fsec;
	char *tzn;
	/* end tax */
	struct pg_tm tm;
	char * res = palloc(DATELEN);
	
	timestamp2tm(ts, &tzp, &tm, &fsec , &tzn, NULL);
	sprintf(res, "%d-%.2d-%.2d %.2d:%.2d:%.2d", tm.tm_year, tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	return res;
}

void	pp_init_image(PPImage * img, Image * gimg)
{
	char * date;

	memset(img, 0, sizeof(img));
	if(!gimg) return;

	//Data we have in Image
	img->width = gimg->columns;
	img->height = gimg->rows;
	
	//Data read from attributes
	date = gm_image_getattr(gimg, ATTR_TIME);
	if(date) {
		img->date = pp_str2timestamp(date);
    } else {
    	TIMESTAMP_NOBEGIN(img->date);
    }
}

FILE *	lo_copy_out(Oid loid)
{
	FILE * lc = tmpfile();
	int			nbytes, tmp;
	char		buf[BUFSIZE];

	LargeObjectDesc * lod = inv_open(loid, INV_READ, CurrentMemoryContext);

	while ((nbytes = inv_read(lod, buf, BUFSIZE)) > 0)
	{
		tmp = fwrite(buf, 1, nbytes, lc);
		if (tmp != nbytes)
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_OBJECT),
                 errmsg("error reading from large object: %d", loid)));
	}
	inv_close(lod);
	//let's reset lc
	//fseek(lc, 0, SEEK_SET);
	return lc;
}
