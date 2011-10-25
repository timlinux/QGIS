#ifndef __QGSREGIONCORRELATOR_H__
#define __QGSREGIONCORRELATOR_H__

#include <fftw3.h>

class QgsRegionCorrelator 
{ 
public:
    struct QgsRegion
    {
        unsigned int *data;
        int width;
        int height;
    };

public:
    QgsRegionCorrelator(int szX, int sxY, int usfac = 10);
    ~QgsRegionCorrelator();

    void findRegion(int bits, double &colShift, double &rowShift, double *nrmsError = NULL, double *diffPhase = NULL);

    inline int getRegionWidth() { return m_region_width; }
    inline int getRegionHeight() { return m_region_height; }

    inline unsigned int *getRegion1() { return m_region1_data; }
    inline unsigned int *getRegion2() { return m_region2_data; }

private:
    int m_usfac;

    int m_region_width;
    int m_region_height;

    unsigned int *m_region1_data;
    unsigned int *m_region2_data;

    fftw_complex *m_buf1ft;
    fftw_complex *m_buf2ft;
    fftw_complex *m_buftmp;
    fftw_complex *m_bufcc;
    size_t m_ccsize;

    fftw_plan m_plan1;
    fftw_plan m_plan2;
    fftw_plan m_plan3;
}; // class

#endif /* __QGSREGIONCORRELATOR_H__ */
