#ifndef SANE_BASE_H
#define SANE_BASE_H

#include <QObject>
#include<QStringList>
#include<QString>
#include<sane/sane.h>
#include<tiffio.h>

namespace QSane
{
class SScanner ;

class SDevive
{
public:
    static void init();
    static void exit();
    static QStringList listdevice();
    static SScanner* open(QString dev);
};


class SScanner : public QObject
{
    Q_OBJECT
private:

public:

    ~SScanner();
    QStringList getModeList();
    QString getMode();
    SANE_Status setMode(QString mode);
    SANE_Status setResolution(int res) ;
    int getResolution() ;
    SANE_Status scan(const char * file) ;

    int findOption(QString value);
    void detailOption(int pos);
    void setPaperSizeA4();

    QString getString(int opt);
    int getWord(int opt);
private :
    struct {
        int tl_x =-1;
        int tl_y =-1;
        int br_x =-1;
        int br_y =-1;
    }scan_area;

    bool debug = false ;
    struct {
        double width =0 ;
        double height=0;
    }paper;

    SANE_Handle handle;
    // resolution
    int resolution ;
    int res_pos =-1;
    // scanner mode
    int mode_pos =-1 ;
private:
    SANE_Status scan_page(TIFF *image , int resolution);
    SScanner(SANE_Handle handle);
    SANE_Status setString(int opt, QString value);
    // word value
    SANE_Status setWord(int opt,double value);
    // paper
    void setPageSize();
    friend class SDevive;
};

}
#endif // SANE_BASE_H
