#include "qgslinecorrector.h"

QgsLineCorrector::QgsLineCorrector(QString inputPath, QString outputPath, QString outputRefPath)
{
  mInputPath = inputPath;
  mOutputPath = outputPath;
  mOutputRefPath = outputRefPath;
  thread = new QgsLineCorrectorThread();
  QObject::connect(thread, SIGNAL(progressed(double, double, QString)), this, SLOT(progress(double, double, QString)));
  QObject::connect(thread, SIGNAL(logged(QString)), this, SLOT(log(QString)));
  QObject::connect(thread, SIGNAL(badLinesFound(QList<int>)), this, SLOT(findBadLines(QList<int>)));
}

void QgsLineCorrector::setPhases(QgsLineCorrector::CorrectionType phase1, QgsLineCorrector::CorrectionType phase2, bool recalculate)
{
  thread->setProcess1(phase1);
  thread->setProcess2(phase2);
  thread->setRecalculation(recalculate);
}

void QgsLineCorrector::setCreateMask(bool create)
{
  thread->setCreateMask(create);
}

void QgsLineCorrector::start()
{
  thread->setPaths(mInputPath, mOutputPath, mOutputRefPath);
  thread->start();
}

void QgsLineCorrector::stop()
{
  thread->stop();
}

void QgsLineCorrector::progress(double total, double part, QString description)
{
  emit progressed(total, part, description);
}

void QgsLineCorrector::log(QString value)
{
  emit logged(value);
}

void QgsLineCorrector::findBadLines(QList<int> value)
{
  emit badLinesFound(value);
}

QgsLineCorrectorThread::QgsLineCorrectorThread(QObject *parent)
  : QThread(parent)
{
  mInputImage = NULL;
  mOutputImage = NULL;
  mOutputRefImage = NULL;
  mInputPath = "";
  mOutputPath = "";
  mOutputRefPath = "";
  mColumns = 0;
  mRows = 0;
  mType1 = QgsLineCorrector::LineBased;
  mType2 = QgsLineCorrector::ColumnBased;
  mRecalculate = true;
  mCreateMask = true;
  mCurrentAction = "Progress";
  mProgressDevider = 1;
  mProgress = 0;
  mWasStopped = false;
}

void QgsLineCorrectorThread::setPaths(QString inputPath, QString outputPath, QString outputRefPath)
{
  mInputPath = inputPath;
  mOutputPath = outputPath;
  mOutputRefPath = outputRefPath;
}

void QgsLineCorrectorThread::stop()
{
  mWasStopped = true;
}
void QgsLineCorrectorThread::setProcess1(QgsLineCorrector::CorrectionType type)
{
  mType1 = type;
}

void QgsLineCorrectorThread::setProcess2(QgsLineCorrector::CorrectionType type)
{
  mType2 = type;
}

void QgsLineCorrectorThread::setRecalculation(bool value)
{
  mRecalculate = value;
}

void QgsLineCorrectorThread::setCreateMask(bool create)
{
  mCreateMask = create;
}

void QgsLineCorrectorThread::run()
{
  mProgressDevider = 0;
  if(mType1 == QgsLineCorrector::LineBased)
  {
    mProgressDevider++;
  }
  else if(mType1 == QgsLineCorrector::ColumnBased)
  {
    mProgressDevider += 2;
  }
  if(mType2 == QgsLineCorrector::LineBased)
  {
    mProgressDevider++;
  }
  else if(mType2 == QgsLineCorrector::ColumnBased)
  {
    mProgressDevider += 2;
  }
  
  openImages();
  emit logged("Scanning image for bad line");
  QList<int> errors = detectErrors();
  emit logged("Bad lines detected");
  emit badLinesFound(errors);
  emit logged("Starting with line correction");
  fixErrors(errors);
  closeImages();
  emit logged("<<Finished>>");
}
   
void QgsLineCorrectorThread::openImages()
{
  GDALAllRegister();
  mInputImage = (GDALDataset*) GDALOpen(mInputPath.toAscii().data(), GA_ReadOnly);
  mColumns = mInputImage->GetRasterXSize();
  mRows = mInputImage->GetRasterYSize();
  mOutputImage = mInputImage->GetDriver()->CreateCopy(mOutputPath.toAscii().data(), mInputImage, false, NULL, NULL, NULL);
}

void QgsLineCorrectorThread::closeImages()
{
  if(mInputImage != NULL)
  {
    GDALClose(mInputImage);
    mInputImage = NULL;
  }
  if(mOutputImage != NULL)
  {
    GDALClose(mOutputImage);
    mOutputImage = NULL;
  }
  if(mOutputRefImage != NULL)
  {
    GDALClose(mOutputRefImage);
    mOutputRefImage = NULL;
  }
}
   
int* QgsLineCorrectorThread::readLine(int lineNumber)
{
  int *rasterData = new int[mColumns];
  mInputImage->GetRasterBand(1)->RasterIO(GF_Read, 0, lineNumber, mColumns, 1, rasterData, mColumns, 1, GDT_UInt32, 0, 0);
  return rasterData;
}

int* QgsLineCorrectorThread::readOutLine(int lineNumber)
{
  int *rasterData = new int[mColumns];
  mOutputImage->GetRasterBand(1)->RasterIO(GF_Read, 0, lineNumber, mColumns, 1, rasterData, mColumns, 1, GDT_UInt32, 0, 0);
  return rasterData;
}

int* QgsLineCorrectorThread::readColumn(int columnNumber)
{
  int *rasterData = new int[mRows];
  mInputImage->GetRasterBand(1)->RasterIO(GF_Read, columnNumber, 0, 1, mRows, rasterData, 1, mRows, GDT_UInt32, 0, 0);
  return rasterData;
}

int* QgsLineCorrectorThread::readOutColumn(int columnNumber)
{
  int *rasterData = new int[mRows];
  mOutputImage->GetRasterBand(1)->RasterIO(GF_Read, columnNumber, 0, 1, mRows, rasterData, 1, mRows, GDT_UInt32, 0, 0);
  return rasterData;
}

QList<int> QgsLineCorrectorThread::detectErrors(bool fromOutput)
{
  QList<double> means;
  for(int i = 0; i < mRows; i++)
  {
    int *row;
    if(fromOutput)
    {
      row = readOutLine(i);
    }
    else
    {
      row = readLine(i);
    }
    means.append(mean(row));
    delete [] row;
  }
  LineErrors errors = findErrors(means, ROW_OUTLIERS, true);
  QList<int> corrupt = analyzeFailures(errors.up, errors.down, mRows);
  return corrupt;
}

void QgsLineCorrectorThread::fixErrors(QList<int> badColumns)
{
  if(mType1 == QgsLineCorrector::ColumnBased && !mWasStopped)
  {
    fixColumn(badColumns, mInputPath);
  }
  else if(mType1 == QgsLineCorrector::LineBased)
  {
    fixLine(badColumns);
    mOutputImage->FlushCache();
  }
  if(mRecalculate && !mWasStopped)
  {
    badColumns = detectErrors(true);
  }
  if(mType2 == QgsLineCorrector::ColumnBased && !mWasStopped)
  {
    fixColumn(badColumns, mOutputPath);
  }
  else if(mType2 == QgsLineCorrector::LineBased)
  {
    fixLine(badColumns);
    mOutputImage->FlushCache();
  }
  
  if(mCreateMask && !mWasStopped)
  {
    mOutputRefImage = mInputImage->GetDriver()->CreateCopy(mOutputRefPath.toAscii().data(), mInputImage, false, NULL, NULL, NULL);
    createMask();
  }
  mProgress = 100.0;
  emit progressed(mProgress, mProgress, mCurrentAction);
}

void QgsLineCorrectorThread::fixLine(QList<int> badColumns)
{
    mBadColumns = badColumns;
    mCurrentAction = "Line-based correction progress";
    double progress = 0;
    for(int i = 0; i < mRows && !mWasStopped; i++)
    {
      progress = (i/(mRows*1.0))*100.0;
      emit progressed(mProgress+progress/mProgressDevider, progress, mCurrentAction);
      int *row = readOutLine(i);
      QList<double> newRow;
      for(int j = 0; j < mColumns; j++)
      {
	newRow.append(row[j]);
      }
      LineErrors errors = findErrors(newRow, STANDARD_DEVIATION_COL);
      
      QList<int> failures = analyzeFailures(errors.up, errors.down, mColumns);
      QList<int> oldFailures(failures);
      for(int j = 0; j < badColumns.size(); j++)
      {
	failures.append(badColumns[j]);
      }
      correct(row, failures);
      
      mOutputImage->GetRasterBand(1)->RasterIO(GF_Write, 0, i, mColumns, 1, row, mColumns, 1, GDT_UInt32, 0, 0);
      
      delete [] row;
    }
    mProgress += progress/mProgressDevider;
}

void QgsLineCorrectorThread::readColumnRasterProgressed(double progress)
{
  emit progressed(mProgress+progress/(mProgressDevider*2.0), progress, mCurrentAction);
}

void QgsLineCorrectorThread::writeColumnRasterProgressed(double progress)
{
  emit progressed(mProgress+progress/(mProgressDevider*2.0), progress, mCurrentAction);
}

void QgsLineCorrectorThread::fixColumn(QList<int> badColumns, QString path)
{
    double progress = 0;
    mCurrentAction = "Virtual raster creation progress";
    if(mOutputImage != NULL)
    {
      GDALClose(mOutputImage);
      mOutputImage = NULL;
    }
    QgsMemoryLineRaster mColumnRaster(path);
    QObject::connect(&mColumnRaster, SIGNAL(progressed(double)), this, SLOT(readColumnRasterProgressed(double)));
    mColumnRaster.open();
    while(mColumnRaster.isRunning());
    mProgress += 100.0/(mProgressDevider*2.0);
    mCurrentAction = "Column-based correction progress";
    for(int i = 0; i < mColumns && !mWasStopped; i++)
    {
      progress = (i/(mColumns*1.0))*100.0;
      emit progressed(mProgress+progress/mProgressDevider, progress, mCurrentAction);
      int *column = mColumnRaster.column(1, i);
      QList<double> newColumn;
      for(int j = 0; j < mRows; j++)
      {
	newColumn.append(column[j]);
      }
      LineErrors errors = findErrors(newColumn, STANDARD_DEVIATION_COL);
      QList<int> failures = analyzeFailures(errors.up, errors.down, mRows);
      QList<int> oldFailures(failures);
      for(int j = 0; j < badColumns.size(); j++)
      {
	failures.append(badColumns[j]);
      }
      correct(column, failures);
      
      mColumnRaster.setColumn(1, i, column);
    }
    mProgress += progress/mProgressDevider;
    mCurrentAction = "Virtual raster storage progress";
    QObject::disconnect(&mColumnRaster, SIGNAL(progressed(double)), this, SLOT(readColumnRasterProgressed(double)));
    QObject::connect(&mColumnRaster, SIGNAL(progressed(double)), this, SLOT(writeColumnRasterProgressed(double)));
    mColumnRaster.save(mOutputPath);
    while(mColumnRaster.isRunning());
    mProgress += 100.0/(mProgressDevider*2.0);
    mOutputImage = (GDALDataset*) GDALOpen(mOutputPath.toAscii().data(), GA_Update);
}

void QgsLineCorrectorThread::createMask()
{
  for(int i = 0; i < mRows; i++)
  {
    int *oldRow = readLine(i);
    int *newRow = readOutLine(i);
    for(int j = 0; j < mColumns; j++)
    {
	oldRow[j] = oldRow[j] - newRow[j];
    }
    mOutputRefImage->GetRasterBand(1)->RasterIO(GF_Write, 0, i, mColumns, 1, oldRow, mColumns, 1, GDT_UInt32, 0, 0);
    mOutputRefImage->GetRasterBand(1)->FlushCache();
    delete [] oldRow;
    delete [] newRow;
  }
  mOutputRefImage->FlushCache();
}

int* QgsLineCorrectorThread::calculateMask(QList<int> corrupt, int size)
{
  int *result = new int[size];
  for(int i = 0; i < size; i++)
  {
    if(corrupt.contains(i))
    {
      result[i] = 255;
    }
    else
    {
      result[i] = 0;
    }
  }
  return result;
}

void QgsLineCorrectorThread::correct(int *column, QList<int> corrupt)
{
  for(int j = 0; j < corrupt.size(); j++)
  {
    int row = corrupt[j];
    int i_b = row-1;
    int i = row;
    int i_a = row+1;
    if(i_b < 0)
    {
      i_b = 0;
    }
    while(corrupt.contains(i_a))
    {
      i_a += 1;
    }
    if(i_a > mRows)
    {
      i_a = mRows;
    }
    int top_row = column[i_b];
    int bottom_row = column[i_a];
    int diff = i_a - i_b - 2;
    if(diff == 0)
    {
      column[i] = (top_row + bottom_row)/2;
    }
    else
    {
      for(int k = 0; k < diff; k++)
      {
	column[i+k] = (top_row*(diff-k)+bottom_row*(k+1))/(diff+1);
      }
    }
  }
}

LineErrors QgsLineCorrectorThread::findErrors(QList<double> means, int limit, bool logIt)
{
  LineErrors errors;
  errors.down.append(0);
  errors.up.append(0);
  double pixCurrent = means[0];
  double pixFuture = means[1];
  for(int i = 0; i < means.size()-2; i++)
  {
    double pixPrev = pixCurrent;
    pixCurrent = pixFuture;
    pixFuture = means[i+2];
    if(test(pixPrev, pixCurrent, limit))
    {
      if(logIt)
      {
	emit logged("Down Scanner: Potential bad line "+QString::number(i+1));
      }
      errors.down.append(true);
    }
    else
    {
      errors.down.append(false);
    }
    if(test(pixFuture, pixCurrent, limit))
    {
      if(logIt)
      {
	emit logged("Up Scanner: Potential bad line "+QString::number(i+1));
      }
      errors.up.append(true);
    }
    else
    {
      errors.up.append(false);
    }
  }
  errors.up.append(0);
  errors.down.append(0);
  return errors;
}

bool QgsLineCorrectorThread::test(double x, double y, int limit)
{
  if(y < 10 || x < 10)
  {
    return true;
  }
  else if(y < 50 && x < 50)
  {
    return false;
  }
  else if((fabs(ratio(x,y) - MEAN) / STANDARD_DEVIATION) > limit)
  {
    return true;
  }
  else
  {
    return false;
  }
}

double QgsLineCorrectorThread::ratio(double a, double b)
{
  if(!a && !b)
  {
    return 0;
  }
  else
  {
    return ((double) a - (double) b) / ((double) (a+b));
  }
}

double QgsLineCorrectorThread::mean(int *values)
{
  double result = 0;
  double total = 0;
  for(int i = 0; i < mColumns; i++)
  {
    total += values[i];
  }
  if(mColumns > 0)
  {
    result = ((double) total) / ((double) mColumns);
  }
  return result;
}

QList<int> QgsLineCorrectorThread::analyzeFailures(QList<bool> failUp, QList<bool> failDown, int limit)
{
  int row = 1;
  QList<int> corrupt;
  while(row < limit)
  {
    if(failUp[row-1])
    {
      if(matchPattern0(row, failDown, failUp))
      {
	corrupt.append(row-1);
	row++;
      }
      else if(matchPattern1(row, failDown, failUp))
      {
	corrupt.append(row);
	row += 2;
      }
      else if(matchPattern2(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+1);
	row += 3;
      }
      else if(matchPattern3(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+2);
	row += 4;
      }
      else if(matchPattern4(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+1);
	corrupt.append(row+2);
	row += 4;
      }
      else if(matchPattern5(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+1);
	corrupt.append(row+3);
	row += 5;
      }
      else if(matchPattern6(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+2);
	corrupt.append(row+3);
	row += 5;
      }
      else if(matchPattern7(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+3);
	row += 5;
      }
      else if(matchPattern8(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+3);
	row += 5;
      }
      else if(matchPattern9(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+2);
	corrupt.append(row+4);
	row += 6;
      }
      else if(matchPattern10(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+3);
	corrupt.append(row+4);
	row += 6;
      }
      else if(matchPattern11(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+1);
	corrupt.append(row+4);
	row += 6;
      }
      else if(matchPattern12(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+2);
	corrupt.append(row+4);
	corrupt.append(row+6);
	row += 8;
      }
      else if(matchPattern13(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+2);
	corrupt.append(row+4);
	corrupt.append(row+7);
	row += 9;
      }
      else if(matchPattern14(row, failDown, failUp))
      {
	corrupt.append(row);
	corrupt.append(row+3);
	corrupt.append(row+5);
	corrupt.append(row+7);
	row += 9;
      }
      else
      {
	row++;
      }
    }
    else
    {
      row++;
    }
  }
  return corrupt;
}

bool QgsLineCorrectorThread::matchPattern0(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+3 || d.size() < row+3 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && d[row-1] && !d[row] && !d[row+1] && !d[row+2])
  {
    if (!u[row-2] && u[row-1] && !u[row] && !u[row+1] && !u[row+2])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern1(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+3 || d.size() < row+3 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && !d[row+2])
  {
    if(!u[row-2] && u[row-1] && u[row] && !u[row+1] && !u[row+2])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern2(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+4 || d.size() < row+4 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && !d[row+1] && d[row+2] && !d[row+3])
  {
    if(!u[row-2] && u[row-1] && !u[row] && u[row+1] && !u[row+2] && !u[row+3])
    {
      return true;
    }
  }
  else if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && d[row+2] && !d[row+3])
  {
    if(!u[row-2] && u[row-1] && u[row] && u[row+1] && !u[row+2] && !u[row+3])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern3(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+5 || d.size() < row+5 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && d[row+2] && d[row+3] && !d[row+4])
  {
    if(!u[row-2] && u[row-1] && u[row] && u[row+1] && u[row+2] && !u[row+3] && !u[row+4])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern4(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+5 || d.size() < row+5 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && !d[row+1] && !d[row+2] && d[row+3] && !d[row+4])
  {
    if(!u[row-2] && u[row-1] && !u[row] && !u[row+1] && u[row+2] && !u[row+3] && !u[row+4])
    {
      return true;
    }
  }
  else if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && !d[row+2] && d[row+3] && !d[row+4])
  {
    if(!u[row-2] && u[row-1] &&  u[row] && !u[row+1] && u[row+2] && !u[row+3] && !u[row+4])
    {
      return true;
    }
  }
  else if(!d[row-2] && !d[row-1] && d[row] && !d[row+1] && d[row+2] && d[row+3] && !d[row+4])
  {
    if(!u[row-2] && u[row-1] && !u[row] && u[row+1] && u[row+2] && !u[row+3] && !u[row+4])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern5(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+6 || d.size() < row+6 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && !d[row+1] && d[row+2] && d[row+3] && d[row+4] && !d[row+5])
  {
    if(!u[row-2] && u[row-1] && !u[row] && u[row+1] && u[row+2] && u[row+3] && !u[row+4] && !u[row+5])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern6(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+6 || d.size() < row+6 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && d[row+2] && !d[row+3] && d[row+4] && !d[row+5])
  {
    if(!u[row-2] && u[row-1] && u[row] && u[row+1] && !u[row+2] && u[row+3] && !u[row+4] && !u[row+5])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern7(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+6 || d.size() < row+6 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && d[row+2] &&  d[row+3] && d[row+4] && !d[row+5])
  {
    if(!u[row-2] && u[row-1] && u[row] && u[row+1] && u[row+2] && u[row+3] && !u[row+4] && !u[row+5])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern8(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+6 || d.size() < row+6 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && !d[row+2] && d[row+3] && d[row+4] && !d[row+5])
  {
    if(!u[row-2] && u[row-1] && u[row] && !u[row+1] && u[row+2] && u[row+3] && !u[row+4] && !u[row+5])
    {
      return true;
    }
  }
  else if(!d[row-2] && !d[row-1] && d[row] && !d[row+1] &&  d[row+2] && !d[row+3] && d[row+4] && !d[row+5])
  {
    if(!u[row-2] && u[row-1] && !u[row] && u[row+1] && !u[row+2] && u[row+3] && !u[row+4] && !u[row+5])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern9(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+7 || d.size() < row+7 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && d[row+2] && d[row+3] && d[row+4] && d[row+5] && !d[row+6])
  {
    if(!u[row-2] && u[row-1] && u[row] && u[row+1] && u[row+2] && u[row+3] && u[row+4] && !u[row+5] && !u[row+6])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern10(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+7 || d.size() < row+7 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && !d[row+2] && d[row+3] && !d[row+4] && d[row+5] && !d[row+6])
  {
    if(!u[row-2] && u[row-1] && u[row] && !u[row+1] && u[row+2] && !u[row+3] && u[row+4] && !u[row+5] && !u[row+6])
    {
      return true;
    }
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && !d[row+2] && d[row+3] && d[row+4] && d[row+5] && !d[row+6])
  {
    if(!u[row-2] && u[row-1] && u[row] && !u[row+1] && u[row+2] && u[row+3] && u[row+4] && !u[row+5] && !u[row+6])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern11(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+7 || d.size() < row+7 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && !d[row+1] && d[row+2] && !d[row+3] && d[row+4] && d[row+5] && !d[row+6])
  {
    if(!u[row-2] && u[row-1] && !u[row] && u[row+1] && !u[row+2] && u[row+3] && u[row+4] && !u[row+5] && !u[row+6])
    {
      return true;
    }
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && d[row+2] && !d[row+3] && d[row+4] && d[row+5] && !d[row+6])
  {
    if(!u[row-2] && u[row-1] && u[row] && u[row+1] && !u[row+2] && u[row+3] && u[row+4] && !u[row+5] && !u[row+6])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern12(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+9 || d.size() < row+9 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && d[row+2] && d[row+3] && d[row+4] && d[row+5] && d[row+6] && d[row+7] && !d[row+8])
  {
    if(!u[row-2] && u[row-1] && u[row] && u[row+1] && u[row+2] && u[row+3] && u[row+4] && u[row+5] && u[row+6] && !u[row+7] && !u[row+8])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern13(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+10 || d.size() < row+10 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && d[row+2] && d[row+3] && d[row+4] && d[row+5] && !d[row+6] && d[row+7] && d[row+8] && !d[row+9])
  {
    if(!u[row-2] && u[row-1] && u[row] && u[row+1] && u[row+2] && u[row+3] && u[row+4] && !u[row+5] && u[row+6] && u[row+7] && !u[row+8] && !u[row+9])
    {
      return true;
    }
  }
  return false;
}

bool QgsLineCorrectorThread::matchPattern14(int row, QList<bool> d, QList<bool> u)
{
  if(u.size() < row+10 || d.size() < row+10 || row < 2)
  {
    return false;
  }
  if(!d[row-2] && !d[row-1] && d[row] && d[row+1] && !d[row+2] && d[row+3] && d[row+4] && d[row+5] && d[row+6] && d[row+7] && d[row+8] && !d[row+9])
  {
    if(!u[row-2] && u[row-1] && u[row] && !u[row+1] && u[row+2] && u[row+3] && u[row+4] && u[row+5] && u[row+6] && u[row+7] && !u[row+8] && !u[row+9])
    {
      return true;
    }
  }
  return false;
}
