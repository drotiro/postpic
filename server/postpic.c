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
#include <pg_config.h>
/* -> dates & co */
#include <utils/datetime.h>
#include <pgtime.h>
/* -> large objects */
#include <storage/large_object.h>
#include <libpq/libpq-fs.h>
#include <libpq/be-fsstubs.h>
#include <storage/fd.h>
/* --- */
#include <utils/geo_decls.h>
#if PG_VERSION_NUM >= 90000
#include <utils/bytea.h>
#endif
#include <utils/elog.h>
#include <utils/builtins.h>
#include <utils/array.h>
#include <utils/syscache.h>
/* GM-related includes */
#include <magick/api.h>
/* Others */
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

PG_MODULE_MAGIC;

/* 
 * The datatype we use internally for storing
 * image information
 */
typedef struct {
	char	vl_len_[4];
	/* FIXED_DATA_BEGIN */
	Timestamp	date;
	int4	width;
	int4	height;
	Oid		cspace;
	int4	iso;
	float4	f_number;
	float4	exposure_t;
	float4	focal_l;
	/* FIXED_DATA_END */
	bytea imgdata;
} PPImage;

/* Colorspace Handling */
typedef struct {
	char * name;
	Oid oid;
} PPColorspace;

PPColorspace colorspaces[] = {
	{ "Unknown", InvalidOid },
	{ "RGB",  InvalidOid },
	{ "RGBA",  InvalidOid },
	{ "Gray", InvalidOid },
	{ "sRGB",  InvalidOid },
	{ "CMYK",  InvalidOid },
	{ NULL, InvalidOid }
};

static Oid colorspace_oid;

/* Color representation */
typedef struct {
	Oid	cs;
	unsigned int cd;
} PPColor;

char * tmpdir = "/tmp";

#define CS_UNKNOWN colorspaces[0]
#define CS_RGB colorspaces[1]
#define CS_RGBA colorspaces[2]
#define CS_GRAY colorspaces[3]
#define CS_sRGB colorspaces[4]
#define CS_CMYK colorspaces[5]

/* Some constant */
#define BUFSIZE		8192
#define INTLEN		20
#define DATELEN		20
#define COLORLEN	18
#define VERLEN		128
#define FIXED_DATA_LEN	36
#define ATTR_TIME	"EXIF:DateTimeOriginal"
#define ATTR_EXPT	"EXIF:ExposureTime"
#define ATTR_FNUM	"EXIF:FNumber"
#define ATTR_ISO	"EXIF:ISOSpeedRatings"
#define ATTR_FLEN	"EXIF:FocalLength"
#define	PP_VERSION_RELEASE	0
#define	PP_VERSION_MAJOR	9
#define	PP_VERSION_MINOR	1

/*
 * Helpful macros
 */
#define PG_GETARG_IMAGE(x) (PPImage*) PG_DETOAST_DATUM(PG_GETARG_POINTER(x))

/*
 * Mandatory and factory methods 
 */
Datum	image_in(PG_FUNCTION_ARGS);
Datum	image_out(PG_FUNCTION_ARGS);
Datum	image_send(PG_FUNCTION_ARGS);
Datum	image_recv(PG_FUNCTION_ARGS);
Datum	image_from_large_object(PG_FUNCTION_ARGS);
Datum	image_new(PG_FUNCTION_ARGS);

Datum	color_in(PG_FUNCTION_ARGS);
Datum	color_out(PG_FUNCTION_ARGS);

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
Datum	image_date(PG_FUNCTION_ARGS);
Datum	image_f_number(PG_FUNCTION_ARGS);
Datum	image_exposure_time(PG_FUNCTION_ARGS);
Datum	image_iso(PG_FUNCTION_ARGS);
Datum	image_focal_length(PG_FUNCTION_ARGS);
Datum	image_colorspace(PG_FUNCTION_ARGS);

/*
 * Image processing functions
 */
Datum	image_thumbnail(PG_FUNCTION_ARGS);
Datum	image_square(PG_FUNCTION_ARGS);
Datum	image_resize(PG_FUNCTION_ARGS);
Datum	image_crop(PG_FUNCTION_ARGS);
Datum	image_rotate(PG_FUNCTION_ARGS);
Datum	image_draw_text(PG_FUNCTION_ARGS);
Datum   image_draw_rect(PG_FUNCTION_ARGS);

/*
 * Aggregate functions
 */
Datum   image_index(PG_FUNCTION_ARGS);

/*
 * Internal and GraphicsMagick's
 */
PPImage *	pp_init_image(Image * gimg);
// parsing and formatting
Timestamp	pp_str2timestamp(const char * date);
char *		pp_timestamp2str(Timestamp ts);
char *		pp_varchar2str(VarChar * text);
int			pp_substr2int(char * str, int off, int len);
float4		pp_parse_float(const char * str);
int			pp_parse_int(char * str);
Oid			pp_parse_cstype(const ColorspaceType t);
void		pp_parse_color(const char * str, PPColor * color);
// To deal easily with GraphicsMagick objects
Image *		gm_image_from_lob(Oid loid);
Image *		gm_image_from_bytea(bytea * imgdata);
char *		gm_image_getattr(Image * img, const char * attr);
void *		gm_image_to_blob(Image * timg, size_t * blen, ExceptionInfo * ex);
void		gm_image_destroy(Image *);
PixelPacket *	gm_ppacket_from_color(const PPColor * color);
// large objects processing
void *		lo_readblob(Oid loid, int * len);
int			lo_size(int32 fd);


PG_FUNCTION_INFO_V1(image_in);
Datum	image_in(PG_FUNCTION_ARGS)
{
	PPImage * img;
	Image * gimg;
	bytea * imgdata;
	Datum data;
	
	data = DirectFunctionCall1(byteain, PG_GETARG_DATUM(0));
	imgdata = (bytea *) DatumGetPointer(data);
	gimg = gm_image_from_bytea(imgdata);
	
	img = pp_init_image(gimg);
	gm_image_destroy(gimg);
	PG_RETURN_POINTER(img);
}

PG_FUNCTION_INFO_V1(image_out);
Datum	image_out(PG_FUNCTION_ARGS)
{
	PPImage * img = PG_GETARG_IMAGE(0);
	return DirectFunctionCall1(byteaout, PointerGetDatum(&img->imgdata));
}

PG_FUNCTION_INFO_V1(image_from_large_object);
Datum	image_from_large_object(PG_FUNCTION_ARGS)
{
	Oid loid = PG_GETARG_OID(0);
	Image * gimg = gm_image_from_lob(loid);
	PPImage * img;
	
	img = pp_init_image(gimg);
	gm_image_destroy(gimg);
	PG_RETURN_POINTER(img);
}

PG_FUNCTION_INFO_V1(image_send);
Datum	image_send(PG_FUNCTION_ARGS)
{
	PPImage * img = PG_GETARG_IMAGE(0);
	PG_RETURN_BYTEA_P(&img->imgdata);
}

PG_FUNCTION_INFO_V1(image_recv);
Datum   image_recv(PG_FUNCTION_ARGS)
{
	PPImage * img;
    Image * gimg;
	Datum data;
	bytea * imgdata;
	
	data = DirectFunctionCall1(bytearecv,PG_GETARG_DATUM(0));
	imgdata  = (bytea *) DatumGetPointer(data);
    gimg = gm_image_from_bytea(imgdata);

    img = pp_init_image(gimg);
    gm_image_destroy(gimg);
	PG_RETURN_POINTER(img);
}

PG_FUNCTION_INFO_V1(image_new);
Datum   image_new(PG_FUNCTION_ARGS)
{
    PPImage * res;
    Image * gimg, * timg;
    PPColor * color;
    int4 w, h;
    ExceptionInfo ex;
    ImageInfo * iinfo;
    char * map;
    Oid ccs;

	w = PG_GETARG_INT32(0);
	h = PG_GETARG_INT32(1);
	color = (PPColor*) PG_GETARG_POINTER(2);
	GetExceptionInfo(&ex);

	iinfo = CloneImageInfo(NULL);
	strcpy(iinfo->magick, "JPEG");
	ccs = color->cs;
	if(ccs == CS_RGB.oid || ccs == CS_sRGB.oid) {
		map = "PRGB";
		iinfo->colorspace = RGBColorspace;
    } else if(ccs == CS_RGBA.oid) {
    	map = "RGBA";
    	iinfo->colorspace = TransparentColorspace;
	} else if(ccs == CS_CMYK.oid) {
		map = "CMYK";
		iinfo->colorspace = CMYKColorspace;
	} else if(ccs == CS_GRAY.oid) {
		map = "PPPI";
		iinfo->colorspace = GRAYColorspace;
	} else {
		elog(ERROR, "Unsupported colorspace %d", color->cs);
		/* Not needed, just to make the compiler happy */
		PG_RETURN_NULL();
	}
	timg = ConstituteImage( 1, 1, // w x h
                     map, // data interpretation
                     CharPixel,
                     &color->cd, // actual pixel data
                     &ex );
	gimg = AllocateImage(iinfo);
	gimg->columns = w;
	gimg->rows = h;
	TransformColorspace(gimg, iinfo->colorspace);
	TextureImage(gimg, timg);

    res = pp_init_image(gimg);
    gm_image_destroy(gimg);
	gm_image_destroy(timg);
    DestroyImageInfo(iinfo);
    DestroyExceptionInfo(&ex);
    PG_RETURN_POINTER(res);
}


PG_FUNCTION_INFO_V1(color_in);
Datum	color_in(PG_FUNCTION_ARGS)
{
	char * cstr;
	PPColor * color;
	
	cstr = PG_GETARG_CSTRING(0);
	color = palloc(sizeof(PPColor));
	pp_parse_color(cstr, color);
	
	PG_RETURN_POINTER(color);
}

PG_FUNCTION_INFO_V1(color_out);
Datum   color_out(PG_FUNCTION_ARGS)
{
	int i = 1;
	char * str = palloc (COLORLEN);
	PPColor * color = (PPColor*) PG_GETARG_POINTER(0);
	while (colorspaces[i].name && color->cs!=colorspaces[i].oid) ++i;
	if (!(colorspaces[i].name)) i=0;
	sprintf(str, "%s#%x", colorspaces[i].name, ntohl(color->cd));
	PG_RETURN_CSTRING(str);
}

PG_FUNCTION_INFO_V1(image_thumbnail);
Datum   image_thumbnail(PG_FUNCTION_ARGS)
{
	ExceptionInfo ex;
	int4 size, sx, sy;
	PPImage * img, *res;
	Image * gimg, * timg;
	
	img = PG_GETARG_IMAGE(0);
	size = PG_GETARG_INT32(1);
	gimg = gm_image_from_bytea(&img->imgdata);
	if(img->width >= img->height) {
		sx = size;
		sy = img->height * size / img->width;
	} else {
		sy = size;
		sx = img->width * size / img->height;
	}
	GetExceptionInfo(&ex);
	timg = ThumbnailImage(gimg, sx, sy, &ex);
	res = pp_init_image(timg);
			
	gm_image_destroy(gimg);
	gm_image_destroy(timg);
	DestroyExceptionInfo(&ex);
	PG_RETURN_POINTER(res);	
}

PG_FUNCTION_INFO_V1(image_resize);
Datum   image_resize(PG_FUNCTION_ARGS)
{
	ExceptionInfo ex;
	int4 sx, sy;
	PPImage * img, * res;
	Image * gimg, * timg;
	
	img = PG_GETARG_IMAGE(0);
	sx = PG_GETARG_INT32(1);
	sy = PG_GETARG_INT32(2);

	gimg = gm_image_from_bytea(&img->imgdata);
	GetExceptionInfo(&ex);
	timg = ResizeImage(gimg, sx, sy, CubicFilter, 1, &ex);
	res = pp_init_image(timg);

	gm_image_destroy(gimg);
	gm_image_destroy(timg);
	DestroyExceptionInfo(&ex);
	PG_RETURN_POINTER(res);	
}

PG_FUNCTION_INFO_V1(image_crop);
Datum   image_crop(PG_FUNCTION_ARGS)
{
	ExceptionInfo ex;
	RectangleInfo rect;
	int4 cx, cy, cw, ch;
	PPImage * img, *res;
	Image * gimg, * timg;
	
	img = PG_GETARG_IMAGE(0);
	cx = PG_GETARG_INT32(1);
	cy = PG_GETARG_INT32(2);
	cw = PG_GETARG_INT32(3);
	ch = PG_GETARG_INT32(4);
	rect.x = cx; rect.y = cy;
	rect.width = cw;
	rect.height = ch;

	gimg = gm_image_from_bytea(&img->imgdata);
	GetExceptionInfo(&ex);
	timg = CropImage(gimg, &rect, &ex);
	res = pp_init_image(timg);

	gm_image_destroy(gimg);
	gm_image_destroy(timg);
	DestroyExceptionInfo(&ex);
	PG_RETURN_POINTER(res);	
}

PG_FUNCTION_INFO_V1(image_rotate);
Datum   image_rotate(PG_FUNCTION_ARGS)
{
	ExceptionInfo ex;
	float4 deg;
	PPImage * img, *res;
	Image * gimg, * timg;
	
	img = PG_GETARG_IMAGE(0);
	deg = PG_GETARG_FLOAT4(1);

	gimg = gm_image_from_bytea(&img->imgdata);
	GetExceptionInfo(&ex);
	timg = RotateImage(gimg, deg, &ex);
	res = pp_init_image(timg);

	gm_image_destroy(gimg);
	gm_image_destroy(timg);
	DestroyExceptionInfo(&ex);
	PG_RETURN_POINTER(res);
}

PG_FUNCTION_INFO_V1(image_square);
Datum   image_square(PG_FUNCTION_ARGS)
{
	ExceptionInfo ex;
	RectangleInfo ri;
	int4 size, sx, sy;
	PPImage * img, * res;
	Image * gimg, * timg, * simg;
	
	img = PG_GETARG_IMAGE(0);
	size = PG_GETARG_INT32(1);
	gimg = gm_image_from_bytea(&img->imgdata);

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
	res = pp_init_image(simg);
	
	gm_image_destroy(gimg);
	gm_image_destroy(timg);
	gm_image_destroy(simg);
	DestroyExceptionInfo(&ex);
	PG_RETURN_POINTER(res);		
}

PG_FUNCTION_INFO_V1(image_draw_text);
Datum   image_draw_text(PG_FUNCTION_ARGS)
{
    PPImage * img, * res;
    PPColor * color = NULL;
    Image * gimg;
    DrawContext ctx;
    VarChar * label, * f_family = NULL;
    char * str;
    int x = 20, y = 20, f_size = 10;
    unsigned int na;
    
    na = PG_NARGS();    
    img = PG_GETARG_IMAGE(0);
    label = PG_GETARG_VARCHAR_P(1);
    switch(na) {
    	case 7: //color
    		color = (PPColor*) PG_GETARG_POINTER(6);
    		/* fallthrough */
    	case 6: //font family and size
    		f_family = PG_GETARG_VARCHAR_P(4);
    		f_size = PG_GETARG_INT32(5);
    		/* fallthrough */
		case 4: // x and y position
			x = PG_GETARG_INT32(2);
			y = PG_GETARG_INT32(3);
    }
    gimg = gm_image_from_bytea(&img->imgdata);
    
    ctx  = DrawAllocateContext((DrawInfo*)NULL, gimg);
    if (na>=6) {
		str = pp_varchar2str(f_family);
    	DrawSetFontFamily(ctx, str);
    	DrawSetFontSize(ctx, f_size);
    	if (na>=7) DrawSetFillColor(ctx, gm_ppacket_from_color(color));
    }
	str = pp_varchar2str(label);
    DrawAnnotation(ctx, x, y, (unsigned char*) str);
    DrawRender(ctx);
    DrawDestroyContext(ctx);
    res = pp_init_image(gimg);
    gm_image_destroy(gimg);

	PG_RETURN_POINTER(res);
}

PG_FUNCTION_INFO_V1(image_draw_rect);
Datum   image_draw_rect(PG_FUNCTION_ARGS)
{
    PPImage * img, * res;
    PPColor * color = NULL;
    Image * gimg;
    DrawContext ctx;
    BOX * rect;
    //bool fill;
    
    img = PG_GETARG_IMAGE(0);
    rect = PG_GETARG_BOX_P(1);
    //fill = PG_GETARG_BOOL(2);
    color = (PPColor*) PG_GETARG_POINTER(2);
    
    gimg = gm_image_from_bytea(&img->imgdata);
    
    ctx  = DrawAllocateContext((DrawInfo*)NULL, gimg);
    DrawSetFillColor(ctx, gm_ppacket_from_color(color));
    DrawRectangle(ctx, rect->high.x, rect->high.y,
		rect->low.x, rect->low.y);
    DrawRender(ctx);
    DrawDestroyContext(ctx);
    res = pp_init_image(gimg);
    gm_image_destroy(gimg);

	PG_RETURN_POINTER(res);
}

PG_FUNCTION_INFO_V1(image_index);
Datum	image_index(PG_FUNCTION_ARGS)
{
	ArrayType * aimgs;
	Image * gimg, * rimg;
	int4 tile, offset;
	int sz, nimgs, i, istart;
	ImageInfo iinfo;
	MontageInfo minfo;
	ExceptionInfo ex;
	PPImage * ptr, *res;
	char * str, * data;
	VarChar * title;
	
	aimgs = PG_GETARG_ARRAYTYPE_P(0);
	title = PG_GETARG_VARCHAR_P(1);
	tile = PG_GETARG_INT32(2);
	
	nimgs = ARR_DIMS(aimgs)[0];
	if(nimgs==0) PG_RETURN_NULL();
	istart = ARR_LBOUND(aimgs)[0]-1;
	data =  ARR_DATA_PTR(aimgs);
	gimg = NewImageList();
	for(i = istart; i < istart+nimgs; ++i) {
		ptr = (PPImage *) data;
		AppendImageToList(&gimg, gm_image_from_bytea(& ptr->imgdata));
		/* go to next image, taking padding into account */
		sz = VARSIZE(data);
		offset = sz; if (sz % 4) offset += (4 - (sz % 4));
		data += offset;
	}
	
	GetImageInfo(&iinfo);
	GetMontageInfo(&iinfo, &minfo);
	minfo.geometry = pstrdup("+4+4");
	str = palloc(2*INTLEN);
	sprintf(str, "%dx%d", tile, (nimgs % tile ? nimgs/tile+1 : nimgs/tile));
	minfo.tile = str;
	str = pp_varchar2str(title);
	minfo.title = str;
	minfo.shadow=1;
	GetExceptionInfo(&ex);
	rimg = MontageImages(gimg, &minfo, &ex);
	CatchException(&ex);
	
	//So sad, it doesn't work if I don't write the img :((
	sprintf(rimg->filename, "%s/ppm%d_%s.jpg", tmpdir, tile, minfo.title );
	str = pstrdup(rimg->filename);
	WriteImage(&iinfo, rimg);
    CatchException(&ex);
    if(!rimg) {
		res = NULL;
    	elog(WARNING, "Can't get montage data");
	}
    else {
    	res = pp_init_image(rimg);
	}

	DestroyImageList(gimg);
	DestroyExceptionInfo(&ex);
	gm_image_destroy(rimg);
	unlink(str);
	if(res) PG_RETURN_POINTER(res);
	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(postpic_version);
Datum	postpic_version(PG_FUNCTION_ARGS)
{
	char * version = palloc(DATELEN);
	sprintf(version, "PostPic version %d.%d.%d", PP_VERSION_RELEASE,
		PP_VERSION_MAJOR, PP_VERSION_MINOR);
	PG_RETURN_CSTRING( version ); 
}

PG_FUNCTION_INFO_V1(postpic_version_release);
Datum	postpic_version_release(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32( PP_VERSION_RELEASE );
}

PG_FUNCTION_INFO_V1(postpic_version_major);
Datum	postpic_version_major(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32( PP_VERSION_MAJOR );
}

PG_FUNCTION_INFO_V1(postpic_version_minor);
Datum	postpic_version_minor(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32( PP_VERSION_MINOR );
}

PG_FUNCTION_INFO_V1(image_width);
Datum	image_width(PG_FUNCTION_ARGS)
{
	PPImage * img = PG_GETARG_IMAGE(0);
	PG_RETURN_INT32(img->width);
}

PG_FUNCTION_INFO_V1(image_height);
Datum	image_height(PG_FUNCTION_ARGS)
{
	PPImage * img = PG_GETARG_IMAGE(0);
	PG_RETURN_INT32(img->height);
}

PG_FUNCTION_INFO_V1(image_date);
Datum	image_date(PG_FUNCTION_ARGS)
{
	PPImage * img = PG_GETARG_IMAGE(0);
	if(TIMESTAMP_IS_NOBEGIN(img->date)) PG_RETURN_NULL();
	PG_RETURN_TIMESTAMP(img->date);
}

PG_FUNCTION_INFO_V1(image_f_number);
Datum	image_f_number(PG_FUNCTION_ARGS)
{
    PPImage * img = PG_GETARG_IMAGE(0);
    if(img->f_number > 0) {
    	PG_RETURN_FLOAT4(img->f_number);
	}
	PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(image_exposure_time);
Datum	image_exposure_time(PG_FUNCTION_ARGS)
{
    PPImage * img = PG_GETARG_IMAGE(0);
    if(img->exposure_t > 0) {
        PG_RETURN_FLOAT4(img->exposure_t);
    }
    PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(image_iso);
Datum	image_iso(PG_FUNCTION_ARGS)
{
    PPImage * img = PG_GETARG_IMAGE(0);
    if(img->iso > 0) {
        PG_RETURN_INT32(img->iso);
    }
    PG_RETURN_NULL();
}


PG_FUNCTION_INFO_V1(image_focal_length);
Datum   image_focal_length(PG_FUNCTION_ARGS)
{
	PPImage * img = PG_GETARG_IMAGE(0);
	if(img->focal_l > 0) {
        PG_RETURN_FLOAT4(img->focal_l);
    }
    PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(image_colorspace);
Datum	image_colorspace(PG_FUNCTION_ARGS)
{
	PPImage * img = PG_GETARG_IMAGE(0);
	return ObjectIdGetDatum(img->cspace);
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

Image *		gm_image_from_lob(Oid loid)
{
	ExceptionInfo ex;
	ImageInfo * iinfo;
	Image * res;
	int blen;
	void * blob;

	if(!OidIsValid(loid)) return NULL;
	GetExceptionInfo(&ex);
	iinfo = CloneImageInfo(NULL);
	blob = lo_readblob(loid, &blen);
	res = BlobToImage(iinfo, blob, blen, &ex); 
	DestroyImageInfo(iinfo);
	DestroyExceptionInfo(&ex);

	return res;
}

Image *		gm_image_from_bytea(bytea * imgdata)
{
	ExceptionInfo ex;
	ImageInfo * iinfo;
	Image * res;

	if(!imgdata) return NULL;
	GetExceptionInfo(&ex);
	iinfo = CloneImageInfo(NULL);
	res = BlobToImage(iinfo, VARDATA(imgdata), VARSIZE(imgdata) - VARHDRSZ, &ex);
	if(!res) {
        ereport(ERROR,
        	(errcode(ERRCODE_UNDEFINED_OBJECT),
			 errmsg("error reading image data: %s", ex.reason)));
	}
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

PixelPacket *   gm_ppacket_from_color(const PPColor * color)
{
	PixelPacket * pp = (PixelPacket *)palloc(sizeof(PixelPacket));
	unsigned int data = ntohl(color->cd);
	
	if ((color->cs == CS_RGB.oid) || (color->cs == CS_sRGB.oid)) data = (data << 8);
	pp->red = (data >> 24) & 0xFF;
	pp->green = (data >> 16 ) & 0xFF;
	pp->blue = (data >> 8 ) & 0xFF;
	pp->opacity = data & 0xFF;
	return pp;
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

char*	pp_timestamp2str(Timestamp ts)
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

char * 	pp_varchar2str(VarChar * text)
{
	char * str;
	int sz;

    sz = VARSIZE(text) - VARHDRSZ;
    str = palloc(sz + 1);
    strncpy(str, VARDATA(text), sz);
    str[sz]=0;

	return str;
}

float4	pp_parse_float(const char * str)
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

Oid		pp_parse_cstype(const ColorspaceType t)
{
	switch (t) {
		case RGBColorspace: return CS_RGB.oid;
		case TransparentColorspace: return CS_RGBA.oid;
		case GRAYColorspace: return CS_GRAY.oid;
		case sRGBColorspace: return CS_sRGB.oid;
		case CMYKColorspace: return CS_CMYK.oid;	
		default: return CS_UNKNOWN.oid;
	}
}

void	pp_parse_color(const char * str, PPColor * color)
{
	char cs[COLORLEN], cd[COLORLEN];
	int i = 0;
	
	while(str[i] && str[i]!='#') ++i;
	if(!str[i]) {
		color->cs = InvalidOid;
		return;
	}
	strncpy(cs, str, i); cs[i]=0;
	strcpy(cd, str + i + 1);
	
	color->cd = htonl(strtoll(cd, NULL, 16));
	if(!cs[0]) {
		color->cs = (strlen(cd)>6 ? CS_RGBA.oid : CS_RGB.oid);
	} else {
		i = 1;
		while(colorspaces[i].name && strcmp(cs, colorspaces[i].name)) ++i;
		if(!(colorspaces[i].name)) i=0;
		color->cs = colorspaces[i].oid;
	}
}

PPImage *	pp_init_image(Image * gimg)
{
	char * attr;
	void * blob;
	size_t blen;
	ExceptionInfo ex;
	PPImage * img;

	if(!gimg) return NULL;

	//Get gimg's blob
	GetExceptionInfo(&ex);
	blob = gm_image_to_blob(gimg, &blen, &ex);
	
	// We need room for our fixed data + our header + the bytea (data + header):
	img = (PPImage *) palloc(FIXED_DATA_LEN + blen + 2*VARHDRSZ);
	// Init imgdata with blob
	SET_VARSIZE(&img->imgdata, blen + VARHDRSZ);
	memcpy(VARDATA(&img->imgdata), blob, blen);
	SET_VARSIZE(img, FIXED_DATA_LEN + blen + 2*VARHDRSZ );
	free(blob);

	//Data we have in Image
	img->width = gimg->columns;
	img->height = gimg->rows;
	img->cspace = pp_parse_cstype(gimg->colorspace);
	
	//Data read from attributes
	attr = gm_image_getattr(gimg, ATTR_TIME);
	img->date = pp_str2timestamp(attr);
    attr = gm_image_getattr(gimg, ATTR_FNUM);
   	img->f_number = pp_parse_float(attr);
	attr = gm_image_getattr(gimg, ATTR_EXPT);
   	img->exposure_t = pp_parse_float(attr);
   	attr = gm_image_getattr(gimg, ATTR_ISO);
   	img->iso = pp_parse_int(attr);
   	attr = gm_image_getattr(gimg, ATTR_FLEN);
   	img->focal_l = pp_parse_float(attr);
		
	return img;
}

int	lo_size(int32 fd)
{
	Datum sz;
	
	DirectFunctionCall3(lo_lseek, Int32GetDatum(fd), Int32GetDatum(0),
		Int32GetDatum(SEEK_END));
	sz = DirectFunctionCall1(lo_tell, Int32GetDatum(fd));
	DirectFunctionCall3(lo_lseek, Int32GetDatum(fd), Int32GetDatum(0), 
		Int32GetDatum(SEEK_SET));
	return DatumGetInt32(sz);	
}

void *  lo_readblob(Oid loid, int * blen)
{
	void * buf;
	int size;
	Datum fd_d;
	int32 fd;
	fd_d = DirectFunctionCall2(lo_open, ObjectIdGetDatum(loid), 
		Int32GetDatum(INV_READ));
	fd = DatumGetInt32(fd_d);
	size = lo_size(fd);
	*blen = size;
	buf = palloc(size);
    if(!buf) {
        ereport(ERROR,
        	(errcode(ERRCODE_UNDEFINED_OBJECT),
			 errmsg("error reading from large object: %d", loid)));
    }
	lo_read(fd, buf, size);
	DirectFunctionCall1(lo_close, Int32GetDatum(fd));

	return buf;	
}

// let's make compiler happy
void _PG_init(void);

/* 
 * This function is called when the PostPic extension is loaded
 * into the server.
 * We use this to get colorspace enum's oids from SysCache and
 * save them for later use
 */
void _PG_init()
{
	Oid csOid, pOid;
	int i;
	char * tde;
	
	/* Read colorspace Oids */
	pOid = GetSysCacheOid(NAMESPACENAME,
		CStringGetDatum("public"), 0, 0, 0);
	csOid = GetSysCacheOid(TYPENAMENSP,
		CStringGetDatum("colorspace"), 
		pOid, 0, 0);
	colorspace_oid = csOid;
	for(i = 0; colorspaces[i].name!=NULL; ++i) {
		colorspaces[i].oid = GetSysCacheOid(ENUMTYPOIDNAME, csOid,
			CStringGetDatum(colorspaces[i].name), 0, 0);
	}
	
	/* Initialize tempdir from TMPDIR, if available */
	tde = getenv("TMPDIR");
	if(tde) tmpdir = tde;
}
