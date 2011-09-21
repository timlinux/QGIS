#include "qgsprogressmonitor.h"

void QgsProgressMonitor::SetProgress(double p)
{
    m_Progress = p;
    emit progressed(p);
}

void QgsProgressMonitor::Log(QString msg)
{
    emit logged(msg);
}

bool QgsProgressMonitor::UpdateProgress(double p) {
    SetProgress(m_Progress + p);
    return IsCanceled();
}

bool QgsProgressMonitor::UpdateProgress(double p, QString msg) {
    Log(msg);
    return UpdateProgress(p);
}

void QgsProgressMonitor::Cancel() 
{ 
    m_Canceled = true; 
}

void QgsProgressMonitor::Finish() 
{ 
    m_Finished = true;
    emit finished(0);
}
