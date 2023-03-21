#include "local.h"
#include "sane_base.h"
#include <QDebug>
#include <cstdlib>
#include <sane/sane.h>
#include <sane/saneopts.h>
#include <string.h>

#include <tiff.h>
#include <tiffio.h>

namespace QSane {

bool STiff::check_sane_format(SANE_Parameters* parm)
{
    int prm = parm->format;
    bool res = false;
    switch (prm) {
    case SANE_FRAME_RED:
    case SANE_FRAME_GREEN:
    case SANE_FRAME_BLUE:
        if (parm->depth == 8)
            res = true;
        break;

    case SANE_FRAME_RGBI:
    case SANE_FRAME_RGB:
        if (parm->depth == 16 || parm->depth == 8)
            res = true;

    case SANE_FRAME_IR:
    case SANE_FRAME_GRAY:
        if (parm->depth == 16 || parm->depth == 8 || parm->depth == 1)
            res = true;
    default:
        break;
    }
    return res;
}
void STiff::set_fields(TIFF* image, SANE_Parameters* parm, int resolution)
{
    char buf[20];
    time_t now = time(NULL);

    strftime((char*)buf, 20, "%Y:%m:%d %H:%M:%S", localtime(&now));

    TIFFSetField(image, TIFFTAG_DATETIME, buf);

    /* setup header. height will be dynamically incremented */

    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, parm->pixels_per_line);
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, parm->depth);
    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 1);

    if (parm->depth == 1) {
        TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
        TIFFSetField(image, TIFFTAG_THRESHHOLDING, THRESHHOLD_BILEVEL);
    } else {
        TIFFSetField(
            image, TIFFTAG_SAMPLESPERPIXEL,
            ((8 * parm->bytes_per_line) / parm->pixels_per_line) / parm->depth);

        if (parm->format == SANE_FRAME_GRAY) {
            TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
            TIFFSetField(image, TIFFTAG_THRESHHOLDING, THRESHHOLD_HALFTONE);
#ifdef SANE_HAS_INFRARED
        } else if (parm->format == SANE_FRAME_IR) {
            TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
            TIFFSetField(image, TIFFTAG_THRESHHOLDING, THRESHHOLD_HALFTONE);
        } else if (parm->format == SANE_FRAME_RGBI) {
            TIFFSetField(image, TIFFTAG_EXTRASAMPLES, EXTRASAMPLE_UNSPECIFIED);
#endif
        } else {
            TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        }
    }

    //    qDebug() << " tiff dept is " << parm->depth;
    if (parm->depth == 1) {
        TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX3);
        TIFFSetField(image, TIFFTAG_GROUP3OPTIONS, GROUP3OPT_2DENCODING);
    } else
        TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);

    TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

    TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
    TIFFSetField(image, TIFFTAG_XRESOLUTION, (float)resolution);
    TIFFSetField(image, TIFFTAG_YRESOLUTION, (float)resolution);

    /* XXX build add date */
    TIFFSetField(image, TIFFTAG_SOFTWARE, "129_SANE", __DATE__);
/* not use now
  if (tiff_orientation) {
    if (strcmp(tiff_orientation, "topleft") == 0)
      TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    else if (strcmp(tiff_orientation, "topright") == 0)
      TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPRIGHT);
    else if (strcmp(tiff_orientation, "botright") == 0)
      TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_BOTRIGHT);
    else if (strcmp(tiff_orientation, "botleft") == 0)
      TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);
    else if (strcmp(tiff_orientation, "lefttop") == 0)
      TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_LEFTTOP);
    else if (strcmp(tiff_orientation, "righttop") == 0)
      TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_RIGHTTOP);
    else if (strcmp(tiff_orientation, "rightbot") == 0)
      TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_RIGHTBOT);
    else if (strcmp(tiff_orientation, "leftbot") == 0)
      TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_LEFTBOT);
    else
      printf("unkown orientation: %s\n", tiff_orientation);
  }
// /*                    */
#ifdef SANE_HAS_EVOLVED
    if (strlen(si.vendor))
        TIFFSetField(image, TIFFTAG_MAKE, si.vendor);

    if (strlen(si.model))
        TIFFSetField(image, TIFFTAG_MODEL, si.model);
#endif
}

void STiff::set_hostcomputer(TIFF* image) { (void)image; }
} // namespace QSane
