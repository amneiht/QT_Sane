#include "sane_base.h"
#include <QDebug>
#include <paper.h>
namespace QSane {

void SScanner::setPaperSizeA4()
{
    const struct paper* pi = paperinfo("A4");
    paper.width = paperpswidth(pi);
    paper.height = paperpsheight(pi);
    setPageSize();
}
void SScanner::setPageSize()
{
    if (this->paper.width == 0 || this->paper.height == 0)
        return;
    SANE_Word w = this->paper.width / 72.0 * 10 * 2.54; /* convert to mm */
    SANE_Word h = this->paper.height / 72.0 * 10 * 2.54;
    setWord(this->scan_area.br_x, w);
    setWord(this->scan_area.br_y, h);
    if (debug) {
        qDebug() << "scan_area width:" << w << " heiht " << h;
    }
}
void SScanner::detailOption(int opt)
{
    if (opt == -1) {
        qDebug() << " cannot access opotion";
        return;
    }
    int i = 0;
    const SANE_Option_Descriptor* mode_opt;
    mode_opt = sane_get_option_descriptor(handle, opt);
    switch (mode_opt->constraint_type) {
    case SANE_CONSTRAINT_STRING_LIST:
        while (mode_opt->constraint.string_list[i] != NULL) {
            qDebug() << mode_opt->name << " : " << mode_opt->constraint.string_list[i];
            i++;
        }
        break;
    case SANE_CONSTRAINT_WORD_LIST:
        while (mode_opt->constraint.string_list[i] != NULL) {
            qDebug() << mode_opt->name << " : " << mode_opt->constraint.word_list[i];
            i++;
        }
        break;
    case SANE_CONSTRAINT_RANGE:
        qDebug() << mode_opt->name << " : min =" << mode_opt->constraint.range->min << " max=" << mode_opt->constraint.range->max;
        break;
    default:
        break;
    }
    qDebug() << mode_opt->name << mode_opt->desc;
    qDebug() << mode_opt->name << mode_opt->unit;
}
}
