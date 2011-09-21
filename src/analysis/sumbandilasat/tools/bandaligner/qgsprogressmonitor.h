#ifndef __QGS_PROGRESS_MONITOR_H__
#define __QGS_PROGRESS_MONITOR_H__

#include <QObject>

class ANALYSIS_EXPORT QgsProgressMonitor : public QObject
{
    Q_OBJECT
  
  signals:
    void logged(QString message);
    void progressed(double progress);
    void finished(int status);

  public:
    QgsProgressMonitor() 
      : m_Canceled(false), 
        m_Finished(false), 
        m_Progress(0.0),
        m_WorkUnit(100.0)
    {}
    ~QgsProgressMonitor() {}

  private:
    bool m_Finished;
    bool m_Canceled;
    double m_Progress;
    double m_WorkUnit;

  public:
    inline bool IsCanceled() { return m_Canceled; }
    inline bool IsFinished() { return m_Finished; }
    inline double GetProgress() { return m_Progress; }
    inline double GetWorkUnit() { return m_WorkUnit; }
    inline void SetWorkUnit(double value) { m_WorkUnit = value; }

    void Log(QString msg);
    void SetProgress(double p);
    bool UpdateProgress(double p);
    bool UpdateProgress(double p, QString msg);
    void Cancel();
    void Finish();

  private:
    // Disable copy-constructor
    QgsProgressMonitor(const QgsProgressMonitor&);
    QgsProgressMonitor& operator = (const QgsProgressMonitor&);
};

#endif /* __QGS_PROGRESS_MONITOR_H__ */