#include "qgsimageprocessor.h"

QgsImageProcessor::QgsImageProcessor()
  : QObject()
{
}

QgsImageProcessor::~QgsImageProcessor()
{
  releaseMemory();
}

void QgsImageProcessor::releaseMemory()
{
  if(thread != NULL)
  {
    thread->stop();
    while(!thread->stopped() || thread->isRunning());
    QObject::disconnect(thread, SIGNAL(updated(int, QString, int)), this, SLOT(update(int, QString, int)));
    QObject::disconnect(thread, SIGNAL(updatedColumns(QString)), this, SLOT(updateColumns(QString)));
    QObject::disconnect(thread, SIGNAL(updatedLog(QString)), this, SLOT(updateLog(QString)));
    delete thread;
    thread = NULL;
  }
}

void QgsImageProcessor::start(int start, int stop)
{
  thread = new QgsImageProcessorThread(mInputPath, mOutputPath, mCheckPath, mInFilePath, mOutFilePath, mGainThreshold, mBiasThreshold, start, stop);
  QObject::connect(thread, SIGNAL(updated(int, QString, int)), this, SLOT(update(int, QString, int)));
  QObject::connect(thread, SIGNAL(updatedColumns(QString)), this, SLOT(updateColumns(QString)));
  QObject::connect(thread, SIGNAL(updatedLog(QString)), this, SLOT(updateLog(QString)));
  thread->start();
}

void QgsImageProcessor::stop()
{
  QObject::disconnect(thread, SIGNAL(updated(int, QString, int)), this, SLOT(update(int, QString, int)));
  QObject::disconnect(thread, SIGNAL(updatedColumns(QString)), this, SLOT(updateColumns(QString)));
  QObject::disconnect(thread, SIGNAL(updatedLog(QString)), this, SLOT(updateLog(QString)));
  thread->stop();
  while(!thread->stopped());
  thread->quit();
}

bool QgsImageProcessor::isRunning()
{
  if(thread == NULL)
  {
    return false;
  }
  return thread->isRunning();
}

void QgsImageProcessor::update(int progress, QString action, int totalProgress)
{
  emit updated(progress, action, totalProgress);
}

void QgsImageProcessor::updateColumns(QString column)
{
  emit updatedColumns(column);
}

void QgsImageProcessor::updateLog(QString message)
{
  emit updatedLog(message);
}

void QgsImageProcessor::setInputPath(QString path)
{
  mInputPath = path;
}

void QgsImageProcessor::setOutputPath(QString path)
{
  mOutputPath = path;
}

void QgsImageProcessor::setCheckPath(QString path)
{
  mCheckPath = path;
}

void QgsImageProcessor::setInFilePath(QString path)
{
  mInFilePath = path;
}

void QgsImageProcessor::setOutFilePath(QString path)
{
  mOutFilePath = path;
}

void QgsImageProcessor::setThresholds(double gain, double bias)
{
  mGainThreshold = gain;
  mBiasThreshold = bias;
}
