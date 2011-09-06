#include "qgsimageprocessorthread.h"
#include <cassert>

//Thread

QgsReadingThread::QgsReadingThread(bool leftRight, int start, int stop, int band, int rows, int columns, QgsMemoryRaster *dataset, QList<BadColumn*> *potentialBadLeft, QList<BadColumn*> *potentialBadRight)
{
  mLeftRight = leftRight;
  mBand = band;
  mColumns = columns;
  mRows = rows;
  mStart = start;
  mStop = stop;
  mDataset = dataset;

  mGainThreshold = 0.1;
  mBiasThreshold = 0.3;
  mPotentialBadLeft = potentialBadLeft;
  mPotentialBadRight = potentialBadRight;
  mProgress = 0.0;
  QThread::setTerminationEnabled();
  wasStopped = false;
}

void QgsReadingThread::setThresholds(double gainThreshold, double biasThreshold)
{
  mGainThreshold = gainThreshold;
  mBiasThreshold = biasThreshold;
}

double QgsReadingThread::progress()
{
  return mProgress;
}

void QgsReadingThread::stop()
{
  wasStopped = true;
}

QString QgsReadingThread::logMessage()
{
  if(!log.isEmpty())
  {
    return log.dequeue();
  }
  return "";
}

bool QgsReadingThread::hasLogMessage()
{
  return (!log.isEmpty());
}

void QgsReadingThread::run()
{
  if(!mLeftRight)
  {
    readFromLeft(mBand, mStart, mStop);
  }
  else
  {
    readFromRight(mBand, mStart, mStop);
  }
}

void QgsReadingThread::readFromLeft(int band, int start, int stop)
{ 
  double progressAdd = 100.0/(stop-start);
  if(progressAdd < 0)
  {
    progressAdd *= -1;
  }  

  while(!wasStopped && start < stop)
  {
    double *x = mDataset->column(band, start);
    double *y = mDataset->column(band, start+1);
    double m, c;
    double cov00, cov01, cov11, sumsq;

    gsl_fit_linear(x, 1 /*Progress by 1 through array*/, y, 1 /*Progress by 1 through array*/, mRows, &c, &m, &cov00, &cov01, &cov11, &sumsq);
    int quality = QgsQualityTester::goodQuality(m, c, mGainThreshold, mBiasThreshold);
    if(quality != 2)
    {
      BadColumn *bc = new BadColumn();
      bc->column = start+1;
      bc->m = m;
      bc->c = c;
      bc->fault = quality;
      mPotentialBadLeft->append(bc);
      log.enqueue("Left-Right Scanner: Potential bad column "+QString::number(bc->column)+" (gain="+QString::number(bc->m, 'g', 5)+" bias="+QString::number(bc->c, 'g', 5)+")");
    }
    start++;
    mProgress += progressAdd;
  }
}

void QgsReadingThread::readFromRight(int band, int start, int stop)
{
    printf("HERE 6g readFromRight(band=%d, start=%d, stop=%d) [enter]\n", band,start,stop);

  double progressAdd = 100.0/(stop-start);
  if(progressAdd < 0)
  {
    progressAdd *= -1;
  }
  while(!wasStopped && start > stop)
  {
    double *x = mDataset->column(band, start);
    double *y = mDataset->column(band, start-1);
    double m, c;
    double cov00, cov01, cov11, sumsq;

    //printf("HERE 6g readFromRight (band=%d, i=%d)\n", band,start);

    gsl_fit_linear(x, 1 /*Progress by 1 through array*/, y, 1 /*Progress by 1 through array*/, mRows, &c, &m, &cov00, &cov01, &cov11, &sumsq);
    int quality = QgsQualityTester::goodQuality(m, c, mGainThreshold, mBiasThreshold);
    if(quality != 2)
    {
      BadColumn *bc = new BadColumn();
      bc->column = start-1;
      bc->m = m;
      bc->c = c;
      bc->fault = quality;
      mPotentialBadRight->append(bc);
      log.enqueue("Right-Left Scanner: Potential bad column "+QString::number(bc->column)+" (gain="+QString::number(bc->m, 'g', 5)+" bias="+QString::number(bc->c, 'g', 5)+")");
    }
    start--;
    mProgress += progressAdd;
  }
  printf("HERE 6g readFromRight [exit]\n");
}

QgsCorrectionThread::QgsCorrectionThread(int start, int stop, int band, int rows, int columns, QgsMemoryRaster *dataset, QList< QList<BadColumn*> > *finalFaults, QgsMemoryRaster *newRaster)
{
  mBand = band;
  mColumns = columns;
  mRows = rows;
  mStart = start;
  mStop = stop;
  mDataset = dataset;
  mFinalFaults = finalFaults;
  mOutset = newRaster;
  QThread::setTerminationEnabled();
  wasStopped = false;
}

void QgsCorrectionThread::setThresholds(double gainThreshold, double biasThreshold)
{
  mGainThreshold = gainThreshold;
  mBiasThreshold = biasThreshold;
}

double QgsCorrectionThread::progress()
{
  return mProgress;
}

void QgsCorrectionThread::stop()
{
  wasStopped = true;
}

QString QgsCorrectionThread::logMessage()
{
  if(!log.isEmpty())
  {
    return log.dequeue();
  }
  return "";
}

bool QgsCorrectionThread::hasLogMessage()
{
  return (!log.isEmpty());
}

void QgsCorrectionThread::run()
{
  assert(mFinalFaults->size() > 0);

  double progressAdd = 100.0/(mStop-mStart);
  for(int i = mStart; i < mStop && !wasStopped; i++)
  {
    if (mFinalFaults->size() < mBand) {
        printf("*** WARNING: skipping correction because no faults available!\n");
        continue;
    }

    BadColumn *bc = mFinalFaults->at(mBand-1).at(i);

    double *oldData = mDataset->column(mBand, bc->column);
    double *postData = mDataset->column(mBand, bc->column+1);
    
    double oldAvg = 0.0;
    double postAvg = 0.0;
    for(int j = 0; j < mRows; j++)
    {
      oldAvg += oldData[j];
      postAvg += postData[j];
    }
    oldAvg /= mRows;
    postAvg /= mRows;

    double *newData = new double[mRows];
    
      double newAvg = 0.0;
      for(int j = 0; j < mRows; j++)
      {
	newData[j] = oldData[j] - (((oldData[j]*bc->m) + bc->c) - oldData[j]);
	newAvg += newData[j];
      }
      newAvg /= mRows;
    
    //delete [] oldData;
    //oldData = NULL;
    //delete [] postData;
    //postData = NULL;

    if((newAvg-postAvg) < (oldAvg-postAvg) && bc->c > 0)
    {
      //int *writeData = new int[mRows];
      //for(int j = 0; j < mRows; j++)
      //{
	//writeData[j] = (int) newData[j];
      //}
      mOutset->setColumn(mBand, bc->column, newData);
      log.enqueue("Column "+QString::number(bc->column)+" was fixed");
    }
    delete [] newData;
    //delete bc;
    //bc = NULL;
    mProgress += progressAdd;
  }
}

QgsImageProcessorThread::QgsImageProcessorThread(QString inputPath, QString outputPath, QString checkPath, QString inPath, QString outPath, double gainThreshold, double biasThreshold, int start, int stop)
{
  mInputPath = inputPath;
  mOutputPath = outputPath;
  mCheckPath = checkPath;
  mInFilePath = inPath;
  mOutFilePath = outPath;
  mGainThreshold = gainThreshold;
  mBiasThreshold = biasThreshold;
  dataset = NULL;
  outputDataset = NULL;
  previousProgress = 0;
  totalProgress = 0;
  totalTasks = 2;
  if(mOutputPath != "")
  {
    totalTasks += 3;
  }
  if(mCheckPath != "")
  {
    totalTasks += 2;
  }
  mStart = start;
  mStop = stop;
  mProgressAction = "";
  rt1 = NULL;
  rt2 = NULL;
  ct1 = NULL;
  ct2 = NULL;
  QThread::setTerminationEnabled();
  wasStopped = false;
}

QgsImageProcessorThread::~QgsImageProcessorThread()
{
   releaseMemory();
}

void QgsImageProcessorThread::handleProgress(double progress, QString action)
{
  QString k = QString::number(progress, 'g', 5);
  double p = k.toDouble();
  if(mProgressAction != "" && action != mProgressAction)
  {
    totalProgressBase += (int) 100.0/totalTasks;
  }
  mProgressAction = action;
  if(p >= previousProgress+1)
  {
    previousProgress = p;
    totalProgress = totalProgressBase + previousProgress/totalTasks;
    emit updated(previousProgress, mProgressAction, totalProgress);
  }
  else
  {
    previousProgress = (int) p;
  }
}

void QgsImageProcessorThread::handleLog(QString message)
{
  if(message != "")
  {
    emit updatedLog(message);
  }
}

void QgsImageProcessorThread::run()
{
  totalProgress = 0;
  totalProgressBase = 0;
  openDataset(mInputPath);

  double startLeft = mStart;
  double startRight = mStop;
  double stopLeft = mStop;
  double stopRight = mStart;
  if(mStart == -1)
  {
    startLeft = 0;
    stopRight = 0;
  }
  if(mStop == -1)
  {
    stopLeft = columns;
    startRight = columns;
  }
  if(!wasStopped && mInFilePath == "")
  {
    for(int bandNumber = 1; bandNumber <= bands; bandNumber++)
    {
      printf("HERE X4 (band# = %d) [enter]\n", bandNumber);

      rt1 = new QgsReadingThread(false, startLeft, stopLeft-1, bandNumber, rows, columns, dataset, &potentialBadLeft, &potentialBadRight);
      rt2 = new QgsReadingThread(true, startRight-1, stopRight, bandNumber, rows, columns, dataset, &potentialBadLeft, &potentialBadRight);
      rt1->setThresholds(mGainThreshold, mBiasThreshold);
      rt2->setThresholds(mGainThreshold, mBiasThreshold);

      printf("HERE X4 (band# = %d) [start]\n", bandNumber);
      previousProgress = 0.0;
#if 0
      rt1->start();
      rt2->start();
      while(rt1->isRunning() || rt2->isRunning())
      {
	handleProgress((rt1->progress()+rt2->progress())/2, "Searching for faulty columns");
	handleLog(rt1->logMessage());
	handleLog(rt2->logMessage());
      }
#else
      /* Looking for a bug, run sequentially */
      rt1->start();
      while(rt1->isRunning()) {
          handleProgress(rt1->progress(), "Searching for faulty columns");
          handleLog(rt1->logMessage());
      }

      rt2->start();
      while(rt2->isRunning()) {
          handleProgress(rt2->progress(), "Searching for faulty columns");
          handleLog(rt2->logMessage());
      }
#endif

      printf("HERE X4 (band# = %d) [cleanup]\n", bandNumber);
      while(rt1->hasLogMessage())
      {
	handleLog(rt1->logMessage());
      }
      while(rt2->hasLogMessage())
      {
	handleLog(rt2->logMessage());
      }
      if(!wasStopped)
      {
	calculateFaults(bandNumber);
      }

      printf("HERE X4 (band# = %d) [exit]\n", bandNumber);
    }
  }
  else if(!wasStopped)
  {
    readFromFile(mInFilePath);
  }
  if(!wasStopped && mOutputPath != "")
  {
    correctFaults();
  }
  if(!wasStopped && mOutFilePath != "")
  {
    writeToFile(mOutFilePath);
  }
  if(!wasStopped && mOutputPath != "" && mCheckPath != "")
  {
    createCheck();
  }
  previousProgress = 0.0;
  handleProgress(100, "Finished");
  releaseMemory();
  handleLog("<<Finished>>");
}

void QgsImageProcessorThread::stop()
{
  if(dataset != NULL)
  {
    dataset->stop();
  }
  if(outputDataset != NULL)
  {
    outputDataset->stop();
  }
  if(rt1 != NULL)
  {
    rt1->stop();
    rt1->quit();
  }
  if(rt2 != NULL)
  {
    rt2->stop();
    rt2->quit();
  }
  if(ct1 != NULL)
  {
    ct1->stop();
    ct1->quit();
  }
  if(ct2 != NULL)
  {
    ct2->stop();
    ct2->quit();
  }
  wasStopped = true;
}

void QgsImageProcessorThread::releaseMemory()
{
  if(dataset != NULL)
  {
    delete dataset;
    dataset = NULL;
  }
  if(outputDataset != NULL)
  {
    delete outputDataset;
    outputDataset = NULL;
  }
  if(rt1 != NULL)
  {
    delete rt1;
    rt1 = NULL;
  }
  if(rt2 != NULL)
  {
    delete rt2;
    rt2 = NULL;
  }
  if(ct1 != NULL)
  {
    delete ct1;
    ct1 = NULL;
  }
  if(ct2 != NULL)
  {
    delete ct2;
    ct2 = NULL;
  }
  for(int i = 0; i < finalFaults.size(); i++)
  {
    for(int j = 0; j < finalFaults[i].size(); j++)
    {
      if(finalFaults[i][j] != NULL)
      {
	delete finalFaults[i][j];
	finalFaults[i][j] = NULL;
      }
    }
  }
  for(int i = 0; i < potentialBadLeft.size(); i++)
  {
    if(potentialBadLeft[i] != NULL)
    {
      delete [] potentialBadLeft[i];
      potentialBadLeft[i] = NULL;
    }
  }
  for(int i = 0; i < potentialBadRight.size(); i++)
  {
    if(potentialBadRight[i] != NULL)
    {
      delete [] potentialBadRight[i];
      potentialBadRight[i] = NULL;
    }
  }
}

bool QgsImageProcessorThread::openDataset(QString inputPath)
{
  dataset = new QgsMemoryRaster(inputPath);
  dataset->open();

  previousProgress = 0.0;
  while(dataset->isRunning())
  {
    handleProgress(dataset->progress(), "Copying raster to memory");
  }

  if(dataset->success())
  {
    bands = dataset->getBands();
    columns = dataset->getColumns();
    rows = dataset->getRows();
    return true;
  }
  return false;
}

void QgsImageProcessorThread::calculateFaults(int band)
{
  QList<BadColumn*> faults;
  for(int left = 0; left < potentialBadLeft.size(); left++)
  {
    for(int right = 0; right < potentialBadRight.size(); right++)
    {
      if(
	left < potentialBadLeft.size()-1 
	&& (potentialBadLeft[left]->column+2) == potentialBadLeft.at(left+1)->column
	&& potentialBadLeft[left]->column+1 == potentialBadRight.at(right)->column
	&& (potentialBadRight[right]->column-2) == potentialBadRight.at(right+1)->column
	)
      {
	BadColumn *bc = new BadColumn();
	bc->column = potentialBadRight[right]->column;
	bc->m = potentialBadRight[right]->m;
	bc->c = potentialBadRight[right]->c;
	faults.append(bc);
	handleLog("Column "+QString::number(bc->column)+" considered bad");
	emit updatedColumns("Band "+QString::number(band)+": "+QString::number(bc->column));
	left++; //ensures that the next column is not tested because it is part of this test (pattern)
	break;
      }
      else if(potentialBadLeft.at(left)->column == potentialBadRight.at(right)->column)
      {
	/*int quality1 = QgsQualityTester::goodQuality(potentialBadLeft.at(right)->m, potentialBadLeft.at(right)->c, mGainThreshold, mBiasThreshold);
	int quality2 = QgsQualityTester::goodQuality(potentialBadRight.at(right)->m, potentialBadRight.at(right)->c, mGainThreshold, mBiasThreshold);
	if(quality1 == quality2)
	{*/
	  BadColumn *bc = new BadColumn();
	  bc->column = potentialBadLeft.at(left)->column;
	  bc->m = potentialBadLeft.at(left)->m;
	  bc->c = potentialBadLeft.at(left)->c;
	  faults.append(bc);
	  handleLog("Column "+QString::number(bc->column)+" considered bad");
	  emit updatedColumns("Band "+QString::number(band)+": "+QString::number(bc->column));
	  break;
	//}
      }
      else if(potentialBadLeft.at(left)->column == potentialBadRight.at(right)->column-2)
      {
	double *x = dataset->column(band, potentialBadLeft.at(left)->column);
	double *y = dataset->column(band, potentialBadLeft.at(left)->column+1);
	double m, c;
	double cov00, cov01, cov11, sumsq;
	gsl_fit_linear(x, 1 /*Progress by 1 through array*/, y, 1 /*Progress by 1 through array*/, rows, &c, &m, &cov00, &cov01, &cov11, &sumsq);
	int quality = QgsQualityTester::goodQuality(m, c, mGainThreshold, mBiasThreshold);
	if(quality != 2)
	{
	  BadColumn *bc = new BadColumn();
	  bc->column = potentialBadLeft.at(left)->column+1;
	  bc->m = m;
	  bc->c = c;
	  faults.append(bc);
	  handleLog("Column "+QString::number(bc->column)+" considered bad");
	  emit updatedColumns("Band "+QString::number(band)+": "+QString::number(bc->column));
	  break;
	}
      }
    }
  }
  for(int left = 0; left < potentialBadLeft.size(); left++)
  {
    bool gotIt = false;
    for(int i = 0; i < faults.size(); i++)
    {
      if(faults.at(i)->column == potentialBadLeft.at(left)->column)
      {
	gotIt = true;
	break;
      }
    }
    if(!gotIt)
    {
      handleLog("Column "+QString::number(potentialBadLeft.at(left)->column)+" considered good");
    }
  }
  for(int right = 0; right < potentialBadRight.size(); right++)
  {
    bool gotIt = false;
    for(int i = 0; i < faults.size(); i++)
    {
      if(faults.at(i)->column == potentialBadRight.at(right)->column)
      {
	gotIt = true;
	break;
      }
    }
    if(!gotIt)
    {
      handleLog("Column "+QString::number(potentialBadRight.at(right)->column)+" considered good");
    }
  }
  /*for(int i = 0; i < potentialBadLeft.size(); i++)
  {
    delete potentialBadLeft.at(i);
  }
  for(int i = 0; i < potentialBadRight.size(); i++)
  {
    delete potentialBadRight.at(i);
  }*/
  finalFaults.append(faults);
}

void QgsImageProcessorThread::correctFaults()
{
  outputDataset = new QgsMemoryRaster(mInputPath);
  outputDataset->open();
  previousProgress = 0.0;
  while(!wasStopped && outputDataset != NULL && outputDataset->isRunning())
  {
    handleProgress(outputDataset->progress(), "Creating new raster");
  }
  if(!wasStopped && outputDataset != NULL && outputDataset->success())
  {
    for(int i = 1; i <= bands; i++)
    {
      correctBand(i, outputDataset);
    }
  }
  if(!wasStopped && outputDataset != NULL)
  {
    outputDataset->writeCopy(mOutputPath);
    previousProgress = 0;
    while(!wasStopped && outputDataset != NULL && outputDataset->isRunning())
    {
      handleProgress(outputDataset->progress(), "Writing output file");
    }
  }
  if (outputDataset != NULL)
  {
    delete outputDataset;
    outputDataset = NULL;
  }
}
  
void QgsImageProcessorThread::correctBand(int band, QgsMemoryRaster *newRaster)
{
  if (finalFaults.size() <= 0) {
      printf("*** WARNING: no band data in finalFaults list. Skipping correction thread!\n");
      return;
  }

  assert(finalFaults.size() > 0);
  int start = 0;
  int stop = finalFaults.at(band-1).size();
  int middle = stop/2;
  ct1 = new QgsCorrectionThread(start, /*middle*/stop, band, rows, columns, dataset, &finalFaults, newRaster);
  //ct2 = new QgsCorrectionThread(middle, stop, band, rows, columns, dataset, &finalFaults, newRaster);
  ct1->setThresholds(mGainThreshold, mBiasThreshold);
  //ct2->setThresholds(mGainThreshold, mBiasThreshold);
  ct1->start();
  //ct2->start();
  previousProgress = 0.0;
  while(ct1->isRunning() )//|| ct2->isRunning())
  {
    handleProgress(outputDataset->progress(), "Correcting raster columns");
    handleLog(ct1->logMessage());
    //handleLog(ct2->logMessage());
  }
  while(ct1->hasLogMessage())
  {
    handleLog(ct1->logMessage());
  }
  //while(ct2->hasLogMessage())
  //{
  //  handleLog(ct2->logMessage());
  //}
  delete ct1;
  //delete ct2;
  ct1 = NULL;
  ct2 = NULL;
}

static bool writeFaultsToFile(QString path, QList< QList<BadColumn*> > &faults)
{
    QFile output(path);

    if (!output.open(QIODevice::WriteOnly))
        return false;

    for(int i = 0; i < faults.size(); i++)
    {
      QList<BadColumn*> badColumns = faults.at(i);

      output.write(QString("Band"+QString::number(i+1)+":\t").toAscii().data());
      for(int j = 0; j < badColumns.size(); j++)
      {
        output.write(QString("[column=" + QString::number(badColumns[j]->column)
            + ",gain=" + QString::number(badColumns[j]->m)
            + ",bias=" + QString::number(badColumns[j]->c)+"]\t").toAscii().data());
      }
      output.write("\n");
    }

    output.close();
    return true;
}

bool QgsImageProcessorThread::writeToFile(QString path)
{
    return writeFaultsToFile(path, finalFaults);
}

bool QgsImageProcessorThread::readFromFile(QString path)
{
  QFile input(path);
  if(input.open(QIODevice::ReadOnly))
  {
    finalFaults.clear();
    int band = 0;
    while(!input.atEnd())
    {
      band += 1;
      QString line = input.readLine();
      int start = line.indexOf(":\t");
      line = line.right(line.length() - (start+1));
      QList<BadColumn*> badColumns;
      while(line.contains("["))
      {
	BadColumn *bc = new BadColumn();
	start = line.indexOf("[");
	int end = line.indexOf("]");
	QString values = line.mid(start+1, end-(start+1));
	int newStart = values.indexOf("column=");
	int newEnd = values.indexOf("gain=");
	bc->column = values.mid(newStart+7, newEnd-(newStart+8)).toInt();
	newStart = values.indexOf("gain=");
	newEnd = values.indexOf("bias=");
	bc->m = values.mid(newStart+5, newEnd-(newStart+6)).toDouble();
	newStart = values.indexOf("bias=");
	bc->c = values.right(values.length()-(newStart+5)).toDouble();
	line = line.right(line.length() - (end+1));
	badColumns.append(bc);
      }
      finalFaults.append(badColumns);
    }
    input.close();
    return true;
  }
  else
  {
    return false;
  }

}

void QgsImageProcessorThread::createCheck()
{
  dataset->writeCheck(mOutputPath, mCheckPath);
  previousProgress = 0.0;
  while(dataset->isRunning())
  {
    handleProgress(dataset->progress(), "Creating check file");
  }
}

bool QgsImageProcessorThread::stopped()
{
  return wasStopped;
}

bool QgsQualityTester::highVariance(double base, double threshold, double value)
{
  double baseThreshold = base - threshold;
  double newValue = value - baseThreshold;
  if (newValue < 0 || newValue > (threshold + threshold))
  {
    return true;
  }
  return false;
}

int QgsQualityTester::goodQuality(double m, double c, double thresholdM, double thresholdC)
{
  bool mGood = false;
  bool cGood = false;
  if(!QgsQualityTester::highVariance(1.0, thresholdM, m))
  {
    mGood = true;
  }
  if(!QgsQualityTester::highVariance(0.0, thresholdC, c))
  {
    cGood = true;
  }
  if (mGood && cGood)
  {
    return 2;
  }
  else if (!mGood)
  {
    return 0;
  }
  return 1;
}
