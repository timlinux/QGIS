#ifndef __QGSREGIONCORRELATOR_H__
#define __QGSREGIONCORRELATOR_H__

namespace QgsRegionCorrelator 
{ 
    struct QgsRegion
    {
        unsigned int *data;
        int width;
        int height;
    };

    struct QgsResult
    {
        QgsResult(bool a, double b = -1, double c = -1, int d = -1, 
                  double e = -1, double f = -1, double g = -1)
        {
            wasSet = a;
            x = b;
            y = c;
            dimension = d;
            match = e;
            confidence = f;
            snr = g;
        }
        bool wasSet;
        double x;
        double y;
        int dimension;
        double match;
        double confidence;
        double snr;
    };

    QgsResult findRegion(QgsRegion &regionRef, QgsRegion &regionNew, int bits);

} // namespace

#endif /* __QGSREGIONCORRELATOR_H__ */
