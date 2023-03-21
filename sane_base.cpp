#include "sane_base.h"

#include <paper.h>
#include <sane/sane.h>
#include <sane/saneopts.h>
#include <stdlib.h>
#include <string.h>
#include <tiff.h>
#include <tiffio.h>

#include "local.h"
#include <QDebug>

static int scanlines = 100;

namespace QSane {

void SDevive::init()
{
    SANE_Int version;
    paperinit();
    sane_init(&version, NULL);
}
void SDevive::exit()
{
    paperdone();
    sane_exit();
}

QStringList SDevive::listdevice()
{
    const SANE_Device** device_list;
    SANE_Status status;
    QStringList res;
    status = sane_get_devices(&device_list, SANE_FALSE);
    if (status != SANE_STATUS_GOOD) {
        printf("sane_get_devices() failed: %s\n", sane_strstatus(status));
        return res;
    }
    for (int i = 0; device_list[i]; i++) {
        QString data = device_list[i]->name;
        res.append(data);
    }
    return res;
}

SScanner* SDevive::open(QString dev)
{
    QByteArray ba = dev.toLocal8Bit();
    const char* c_str2 = ba.data();
    SANE_Handle handle;
    SANE_Status status = sane_open(c_str2, &handle);
    if (status != SANE_STATUS_GOOD) {
        return NULL;
    }
    qDebug() << "open device " << dev << " status ok";
    return new SScanner(handle);
}

SANE_Status SScanner::scan_page(TIFF* image, int resolution)
{
    // copy from tiffsace : https://github.com/dwery/tiffscan
    SANE_Parameters parm;
    SANE_Status status;
    int rows = 0;
    int len;

    SANE_Byte* buffer = nullptr;
    size_t buffer_size;

    //	status = sane_set_io_mode(handle, SANE_TRUE);
    status = sane_start(handle);

    /* return immediately when no docs are available */
    if (status == SANE_STATUS_NO_DOCS) {
        qDebug() << __func__ << ":sane_nodoc ";
        return status;
    }

    if (status != SANE_STATUS_GOOD) {
        qDebug() << __func__ << ":sane_start: " << sane_strstatus(status);
        goto sane_end;
    }
    status = sane_get_parameters(handle, &parm);
    if (status != SANE_STATUS_GOOD) {
        qDebug() << __func__ << ":sane_get_parameters: " << sane_strstatus(status);
        goto sane_end;
    }
    if (!STiff::check_sane_format(&parm)) {
        status = SANE_STATUS_INVAL;
        qDebug() << __func__ << ":format not ok" << sane_strstatus(status);
        goto sane_end;
    }
    // debug
    qDebug() << "pixels_per_line" << parm.pixels_per_line;
    qDebug() << "bytes_per_line" << parm.bytes_per_line;
    buffer_size = scanlines * parm.bytes_per_line;

    buffer = (SANE_Byte*)malloc(buffer_size);
    qDebug() << "start scan";
    /* prepare tiff directory */
    STiff::set_fields(image, &parm, resolution);
    STiff::set_hostcomputer(image);

    while (1) {
        /* read from SANE */

        status = sane_read(handle, buffer, buffer_size, &len);
        if (status == SANE_STATUS_EOF) {
            qDebug() << __func__ << " end of file";
            break;
        }

        if (status != SANE_STATUS_GOOD) {
            qDebug() << "sane_read: " << sane_strstatus(status);
            break;
        }

        /* no data? keep reading */
        if (len == 0)
            continue;
        /* write to file */
        if (1) {
            int i;
            unsigned char* p = buffer;
            int lines = len / parm.bytes_per_line;

            /* Write each scanline */
            for (i = 0; i < lines; i++) {
                TIFFWriteScanline(image, p, rows++, 0);
                p += parm.bytes_per_line;
            }
        }
    }

    if (status == SANE_STATUS_EOF) {
        status = SANE_STATUS_GOOD;
    }
    TIFFWriteDirectory(image);
sane_end:
    qDebug() << "end scan";
    if (buffer)
        free(buffer);
    return status;
}

SScanner::SScanner(SANE_Handle handle)
{
    this->handle = handle;

    // get sane resolution
    SANE_Status status;
    SANE_Int num_dev_options;
    int i;
    const SANE_Option_Descriptor* opt;

    status = sane_control_option(handle, 0, SANE_ACTION_GET_VALUE,
        &num_dev_options, 0);
    if (status != SANE_STATUS_GOOD)
        return;
    for (i = 0; i < num_dev_options; i++) {
        opt = sane_get_option_descriptor(handle, i);
        if (debug)
            qDebug() << "opotion name is " << opt->name;
        if (!SANE_OPTION_IS_SETTABLE(opt->cap)) {
            continue;
        }
        if (opt->type == SANE_TYPE_GROUP)
            continue;

        if (opt->type == SANE_TYPE_BUTTON && !SANE_OPTION_IS_ACTIVE(opt->cap))
            continue;

        if ((opt->type == SANE_TYPE_FIXED || opt->type == SANE_TYPE_INT) && opt->size == sizeof(SANE_Int) && (opt->unit == SANE_UNIT_DPI) && (strcmp(opt->name, SANE_NAME_SCAN_RESOLUTION) == 0)) {
            this->res_pos = i;
        } else if (strcmp(opt->name, SANE_NAME_SCAN_MODE) == 0) {
            this->mode_pos = i;
        } else if (strcmp(opt->name, SANE_NAME_SCAN_TL_X) == 0) {
            this->scan_area.tl_x = i;
        } else if (strcmp(opt->name, SANE_NAME_SCAN_TL_Y) == 0) {
            this->scan_area.tl_y = i;
        } else if (strcmp(opt->name, SANE_NAME_SCAN_BR_X) == 0) {
            this->scan_area.br_x = i;
        } else if (strcmp(opt->name, SANE_NAME_SCAN_BR_Y) == 0) {
            this->scan_area.br_y = i;
        }
    }
}

SANE_Status SScanner::setString(int opt, QString value)
{
    if (opt == -1) {
        qDebug() << " cannot access opotion";
        return SANE_STATUS_ACCESS_DENIED;
    }
    const SANE_Option_Descriptor* mode_opt;
    mode_opt = sane_get_option_descriptor(handle, opt);
    if (mode_opt->constraint_type == SANE_CONSTRAINT_STRING_LIST) {
        QByteArray ba = value.toLocal8Bit();
        const char* c_str2 = ba.data();
        return sane_control_option(handle, this->mode_pos, SANE_ACTION_SET_VALUE, (void*)c_str2, 0);
    } else {
        qDebug() << "unsuport Scanner Mode type String";
    }
    return SANE_STATUS_UNSUPPORTED;
}

QString SScanner::getString(int opt)
{
    QString res;
    if (opt == -1) {
        qDebug() << " cannot access opotion";
        return res;
    }
    char buff[200];
    const SANE_Option_Descriptor* mode_opt;
    mode_opt = sane_get_option_descriptor(handle, opt);
    if (mode_opt->constraint_type == SANE_CONSTRAINT_STRING_LIST) {
        sane_control_option(handle, opt, SANE_ACTION_GET_VALUE, buff, 0);
        res = buff;
    } else {
        qDebug() << "unsuport type";
    }
    return res;
}

SANE_Status SScanner::setWord(int opt, double v)
{
    if (opt == -1) {
        qDebug() << " cannot access opotion";
        return SANE_STATUS_ACCESS_DENIED;
    }
    SANE_Word *p, value;
    const SANE_Option_Descriptor* l_opt;
    l_opt = sane_get_option_descriptor(handle, opt);
    if (l_opt == NULL) {
        qDebug() << " Couldn't get option descriptor for option";
        return SANE_STATUS_INVAL;
    }
    if (l_opt->type == SANE_TYPE_FIXED)
        value = SANE_FIX(v);
    else
        value = (SANE_Word)v;
    p = &value;

    return sane_control_option(handle, opt, SANE_ACTION_SET_VALUE, p, 0);
}

int SScanner::getWord(int opt)
{
    int res = -1;
    if (opt == -1) {
        qDebug() << " cannot access resolution opotion";
        return res;
    }
    const SANE_Option_Descriptor* res_opt;
    res_opt = sane_get_option_descriptor(handle, opt);
    void* val = alloca(res_opt->size + 1);
    sane_control_option(handle, opt, SANE_ACTION_GET_VALUE, val, 0);

    if (res_opt->type == SANE_TYPE_INT)
        res = *(SANE_Int*)val;
    else
        res = (int)(SANE_UNFIX(*(SANE_Fixed*)val) + 0.5);

    return res;
}

SScanner::~SScanner() { sane_close(handle); }

QStringList SScanner::getModeList()
{
    QStringList res;
    if (this->mode_pos == -1) {
        qDebug() << " cannot access scaner mode opotion";
        return res;
    }

    int i = 0;
    const SANE_Option_Descriptor* mode_opt;
    mode_opt = sane_get_option_descriptor(handle, this->mode_pos);
    if (mode_opt != NULL) {
        switch (mode_opt->constraint_type) {
        case SANE_CONSTRAINT_STRING_LIST:
            while (mode_opt->constraint.string_list[i] != NULL) {
                res.append(mode_opt->constraint.string_list[i]);
                if (debug)
                    qDebug() << __func__ << " : " << mode_opt->constraint.string_list[i];
                i++;
            }
            break;
        case SANE_CONSTRAINT_WORD_LIST:
            // todo: handle mode if it is int list
            break;
        default:
            break;
        }
    }
    return res;
}

QString SScanner::getMode()
{
    return getString(this->mode_pos);
}

SANE_Status SScanner::setMode(QString mode)
{
    return setString(this->mode_pos, mode);
}

SANE_Status SScanner::setResolution(int res)
{
    return setWord(this->res_pos, res);
}

int SScanner::getResolution()
{
    return getWord(this->res_pos);
}

SANE_Status SScanner::scan(const char* file)
{
    TIFF* image = NULL;

    SANE_Status status = SANE_STATUS_GOOD;

    int resolution = this->getResolution();

    image = TIFFOpen(file, "w");

    if (image == NULL) {
        printf("cannot open file\n");
        return SANE_STATUS_ACCESS_DENIED;
    }

    do {
        status = this->scan_page(image, resolution);
    } while (status == SANE_STATUS_GOOD);

    sane_cancel(handle);
    if (image) {
        TIFFClose(image);
    }
    return status;
}

int SScanner::findOption(QString value)
{
    int i, num_dev_options;
    int res = -1;
    const SANE_Option_Descriptor* opt;

    SANE_Status status = sane_control_option(handle, 0, SANE_ACTION_GET_VALUE,
        &num_dev_options, 0);
    if (status != SANE_STATUS_GOOD)
        return -1;
    for (i = 0; i < num_dev_options; i++) {
        opt = sane_get_option_descriptor(handle, i);
        if (debug)
            qDebug() << "opotion name is " << opt->name;
        if (!SANE_OPTION_IS_SETTABLE(opt->cap)) {
            continue;
        }
        if (QString::compare(value, opt->name) == 0) {
            res = i;
            break;
        }
    }
    return res;
}

} // namespace QSane
