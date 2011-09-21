
class QgsProgressMonitor : QObject
{

%TypeHeaderCode
#include <qgsprogressmonitor.h>
%End

  signals:

    void progressed(double progress);
    void logged(QString message);
    void finished(int status);

  public:

    QgsProgressMonitor();
    
    bool IsCanceled();
    void Cancel();

    bool IsFinished();
    void Finish();
};
