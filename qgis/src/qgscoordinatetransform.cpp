/***************************************************************************
               QgsCoordinateTransform.cpp  - Coordinate Transforms
                             -------------------
    begin                : Dec 2004
    copyright            : (C) 2004 Tim Sutton
    email                : tim at linfiniti.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /* $Id$ */
#include <qtextstream.h>
#include "qgscoordinatetransform.h"
#include "qgsspatialreferences.h"

QgsCoordinateTransform::QgsCoordinateTransform( QString theSourceWKT, QString theDestWKT ) : QObject()
{
  mSourceWKT = theSourceWKT;
  mDestWKT = theDestWKT;
  // initialize the coordinate system data structures
  initialise();
}


QgsCoordinateTransform::~QgsCoordinateTransform()
{
  // free the proj objects
  pj_free(mSourceProjection);
  pj_free(mDestinationProjection);
}

void QgsCoordinateTransform::setSourceWKT(QString theWKT)
{
  mSourceWKT = theWKT;
  initialise();
}
void QgsCoordinateTransform::setDestWKT(QString theWKT)
{
#ifdef QGISDEBUG
  std::cout << "QgsCoordinateTransform::setDestWKT called" << std::endl;
#endif
  mDestWKT = theWKT;
  initialise();
}

void QgsCoordinateTransform::initialise()
{
  mInitialisedFlag=false; //guilty until proven innocent...
  //default to geo / wgs84 for now .... later we will make this user configurable
  QString defaultWkt = QgsSpatialReferences::instance()->getSrsBySrid("4326")->srText();
  //default input projection to geo wgs84  
  if (mSourceWKT.isEmpty())
  {
    mSourceWKT = defaultWkt;
  }
  //but default output projection to be the same as input proj
  //whatever that may be...
  if (mDestWKT.isEmpty())
  {
    mDestWKT = mSourceWKT;
  }  

  if (mSourceWKT == mDestWKT)
  {
    mShortCircuit=true;
    return;
  }
  else
  {
    mShortCircuit=false;
  }

  OGRSpatialReference myInputSpatialRefSys, myOutputSpatialRefSys;
  //this is really ugly but we need to get a QString to a char**
  char *mySourceCharArrayPointer = (char *)mSourceWKT.ascii();
  char *myDestCharArrayPointer = (char *)mDestWKT.ascii();

  /* Here are the possible OGR error codes :
     typedef int OGRErr;

#define OGRERR_NONE                0
#define OGRERR_NOT_ENOUGH_DATA     1    --> not enough data to deserialize 
#define OGRERR_NOT_ENOUGH_MEMORY   2
#define OGRERR_UNSUPPORTED_GEOMETRY_TYPE 3
#define OGRERR_UNSUPPORTED_OPERATION 4
#define OGRERR_CORRUPT_DATA        5
#define OGRERR_FAILURE             6
#define OGRERR_UNSUPPORTED_SRS     7 */

  OGRErr myInputResult = myInputSpatialRefSys.importFromWkt( & mySourceCharArrayPointer );
  if (myInputResult != OGRERR_NONE)
  {
    std::cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"<< std::endl;
    std::cout << "The source projection for this layer could *** NOT *** be set " << std::endl;
    std::cout << "INPUT: " << std::endl << mSourceWKT << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
    return;
  }

  OGRErr myOutputResult = myOutputSpatialRefSys.importFromWkt( & myDestCharArrayPointer );
  if (myOutputResult != OGRERR_NONE)
  {
    std::cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"<< std::endl;
    std::cout << "The dest projection for this layer could *** NOT *** be set " << std::endl;
    std::cout << "OUTPUT: " << std::endl << mDestWKT << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
    return;
  }  
  // create the proj4 structs needed for transforming 
  // get the proj parms for source cs
  char *proj4src;
  myInputSpatialRefSys.exportToProj4(&proj4src);
  // store the src proj parms in a QString because the pointer populated by exportToProj4
  // seems to get corrupted prior to its use in the transform
  mProj4SrcParms = proj4src;
  // check to see if we have datum information -- if not it might be an ESRI format WKT
  // XXX this datum check is not appropriate for all CS
    // try getting it as an esri format wkt
    myInputSpatialRefSys.morphFromESRI();
    myInputSpatialRefSys.exportToProj4(&proj4src);
    mProj4SrcParms = proj4src;
  // get the proj parms for dest cs
  char *proj4dest;
  myOutputSpatialRefSys.exportToProj4(&proj4dest);
  // store the dest proj parms in a QString because the pointer populated by exportToProj4
  // seems to get corrupted prior to its use in the transform
  mProj4DestParms = proj4dest;
    // try getting it as an esri format wkt
    myOutputSpatialRefSys.morphFromESRI();
    myOutputSpatialRefSys.exportToProj4(&proj4dest);
    mProj4DestParms = proj4dest;
  // init the projections (destination and source)
  mDestinationProjection = pj_init_plus((const char *)mProj4DestParms);
  mSourceProjection = pj_init_plus((const char *)mProj4SrcParms);

  if ( !mSourceProjection  || ! mDestinationProjection)
  {
    std::cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"<< std::endl;
    std::cout << "The OGR Coordinate transformation for this layer could *** NOT *** be set " << std::endl;
    std::cout << "INPUT: " << std::endl << mSourceWKT << std::endl;
    std::cout << "OUTPUT: " << std::endl << mDestWKT  << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
    return;
  }
  else
  {
    mInitialisedFlag = true;
    std::cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"<< std::endl;
    std::cout << "The OGR Coordinate transformation for this layer was set to" << std::endl;
    std::cout << "INPUT: " << std::endl << mSourceWKT << std::endl;
    std::cout << "PROJ4: " << std::endl << mProj4SrcParms << std::endl;  
    std::cout << "OUTPUT: " << std::endl << mDestWKT  << std::endl;
    std::cout << "PROJ4: " << std::endl << mProj4DestParms << std::endl;  
    std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
  }
}

//
//
// TRANSFORMERS BELOW THIS POINT .........
//
//
//


QgsPoint QgsCoordinateTransform::transform(const QgsPoint thePoint,TransformDirection direction) const
{
  if (mShortCircuit || !mInitialisedFlag) return thePoint;
  // transform x
  double x = thePoint.x(); 
  double y = thePoint.y();
  double z = 0.0;
  try
  {

    transformCoords(1, x, y, z, direction );
  }
  catch(QgsCsException &cse)
  {
    //something bad happened....
    // rethrow the exception
    throw cse;
  }
#ifdef QGISDEBUG 
  //std::cout << "Point projection...X : " << thePoint.x() << "-->" << x << ", Y: " << thePoint.y() << " -->" << y << std::endl;
#endif        
  return QgsPoint(x, y);
} 


QgsPoint QgsCoordinateTransform::transform(const double theX, const double theY=0,TransformDirection direction) const
{
  transform(QgsPoint(theX, theY), direction);
}

QgsRect QgsCoordinateTransform::transform(const QgsRect theRect,TransformDirection direction) const
{
  if (mShortCircuit || !mInitialisedFlag) return theRect;
  // transform x
  double x1 = theRect.xMin(); 
  double y1 = theRect.yMin();
  double x2 = theRect.xMax(); 
  double y2 = theRect.yMax();  

#ifdef QGISDEBUG   

  std::cout << "vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv"<< std::endl;
  std::cout << "Rect  projection..." << std::endl;
  std::cout << "INPUT: " << std::endl << mSourceWKT << std::endl;
  std::cout << "PROJ4: " << std::endl << mProj4SrcParms << std::endl;  
  std::cout << "OUTPUT: " << std::endl << mDestWKT  << std::endl;
  std::cout << "PROJ4: " << std::endl << mProj4DestParms << std::endl;  
  std::cout << "INPUT RECT: " << std::endl << x1 << "," << y1 << ":" << x2 << "," << y2 << std::endl;
  std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
#endif    
  // Number of points to reproject------+
  //                                    | 
  //                                    V 
  try{
    double z = 0.0;
    transformCoords(1, x1, y1, z, direction);
    transformCoords(1, x2, y2, z, direction);

  }
  catch(QgsCsException &cse)
  {
    // rethrow the exception
    throw cse;
  }

#ifdef QGISDEBUG 
  std::cout << "Rect projection..." 
    << "Xmin : " 
    << theRect.xMin() 
    << "-->" << x1 
    << ", Ymin: " 
    << theRect.yMin() 
    << " -->" << y1
    << "Xmax : " 
    << theRect.xMax() 
    << "-->" << x2 
    << ", Ymax: " 
    << theRect.yMax() 
    << " -->" << y2         
    << std::endl;
#endif        
  return QgsRect(x1, y1, x2 , y2);
} 




void QgsCoordinateTransform::transformCoords( const int& numPoints, double& x, double& y, double& z,TransformDirection direction) const
{
  // use proj4 to do the transform   
  QString dir;  
  // if the source/destination projection is lat/long, convert the points to radians
  // prior to transforming
  if((pj_is_latlong(mDestinationProjection) && (direction == INVERSE))
      || (pj_is_latlong(mSourceProjection) && (direction == FORWARD)))
  {
    x *= DEG_TO_RAD;
    y *= DEG_TO_RAD;
    z *= DEG_TO_RAD;

  }
  int projResult;
  if(direction == INVERSE)
  {
    std::cout << "!!!! INVERSE TRANSFORM !!!!" << std::endl; 
    projResult = pj_transform(mDestinationProjection, mSourceProjection , numPoints, 0, &x, &y, &z);
    dir = "inverse";
  }
  else
  {
    std::cout << "!!!! FORWARD TRANSFORM !!!!" << std::endl; 
    projResult = pj_transform(mSourceProjection, mDestinationProjection, numPoints, 0, &x, &y, &z);
    dir = "forward";
  }

  if (projResult != 0) 
  {
    //something bad happened....
    QString msg;
    QTextOStream pjErr(&msg);

    pjErr << tr("Failed") << " " << dir << " " << tr("transform of") << x << ", " <<  y
      << pj_strerrno(projResult) << "\n";
    throw  QgsCsException(msg);
  }
  // if the result is lat/long, convert the results from radians back
  // to degrees
  if((pj_is_latlong(mDestinationProjection) && (direction == FORWARD))
      || (pj_is_latlong(mSourceProjection) && (direction == INVERSE)))
  {
    x *= RAD_TO_DEG;
    y *= RAD_TO_DEG;
    z *= RAD_TO_DEG;
  }
}
