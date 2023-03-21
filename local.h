#ifndef LOCAL_H
#define LOCAL_H

#include <sane/sane.h>
#include <sane/saneopts.h>
#include <tiffio.h>

#ifndef SANE_FRAME_IR
#define SANE_FRAME_IR 0x0F
#endif

#ifndef SANE_FRAME_RGBI
#define SANE_FRAME_RGBI 0x10
#endif

namespace QSane{

    class STiff
    {
    public :
        static bool check_sane_format(SANE_Parameters* parm);
        static void set_fields(TIFF* image, SANE_Parameters* parm, int resolution);
        static void set_hostcomputer(TIFF* image) ;
    };
}
#endif // LOCAL_H
