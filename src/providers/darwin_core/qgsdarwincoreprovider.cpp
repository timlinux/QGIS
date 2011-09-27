#include "qgsdarwincoreprovider.h"
#include "qgsgeometry.h"
#include "qgsspatialindex.h"

const char NS_SEPARATOR = '?';
const QString TAXON_NAMESPACE = "http://rs.tdwg.org/ontology/voc/TaxonOccurrence#";

static const QString TEXT_PROVIDER_KEY = "darwin_core";
static const QString TEXT_PROVIDER_DESCRIPTION = "Darwin core provider";

QgsDarwinCoreProvider::QgsDarwinCoreProvider( const QString& uri ): QgsVectorDataProvider( uri ), mParseMode( Unknown ), BUFFER_SIZE( 10 * 1024 * 1024 ),
mValid( true ), mFinished( false ), mAtEnd( false ), mFetchGeometry( true )
{
  mFieldMap.insert( 0, QgsField("test", QVariant::String ) ); //debug

  //Buffer for file reading operations
  mBuffer = new char[BUFFER_SIZE];

  //normally latlong
  mCrs.createFromEpsg( 4326 );

  mFile.setFileName( uri );
  if( !mFile.open( QIODevice::ReadOnly ) )
  {
    mValid = false;
    return;
  }

  mSpatialIndex = new QgsSpatialIndex;

  mXmlParser = XML_ParserCreateNS( NULL, NS_SEPARATOR );
  memset(mBuffer, 0, BUFFER_SIZE);
  XML_ParserReset( mXmlParser, NULL );
  XML_SetUserData( mXmlParser, this );
  XML_SetElementHandler( mXmlParser, QgsDarwinCoreProvider::start, QgsDarwinCoreProvider::end );
  XML_SetCharacterDataHandler( mXmlParser, QgsDarwinCoreProvider::chars );

  while ( !mFile.atEnd() )
  {
    long int readBytes = mFile.read( mBuffer, BUFFER_SIZE );
    if ( mFile.atEnd() )
    {
      mAtEnd = 1;
    }

    if ( readBytes > 0 )
    {
      XML_Status s = XML_Parse( mXmlParser, mBuffer, readBytes, mAtEnd );
    }
  }

  //create mFieldMap from the encountered attributes
  QHash< QString, QPair<int, QgsField> >::const_iterator fieldIt = mCurrentFields.constBegin();
  for(; fieldIt != mCurrentFields.constEnd(); ++fieldIt )
  {
    mFieldMap.insert( fieldIt.value().first, fieldIt.value().second );
  }
}

QgsDarwinCoreProvider::~QgsDarwinCoreProvider()
{
  deleteCachedFeatures();
  delete[] mBuffer;
  mFile.close();
  delete mSpatialIndex;
}

void QgsDarwinCoreProvider::deleteCachedFeatures()
{
  QHash<int, QgsFeature*>::iterator it = mFeatures.begin();
  for(; it != mFeatures.end(); ++it )
  {
    delete it.value();
  }
  mFeatures.clear();
}

void QgsDarwinCoreProvider::select( QgsAttributeList fetchAttributes,
                                    QgsRectangle rect, bool fetchGeometry, bool useIntersect )
{
  mFetchRectangle = rect;
  mFetchGeometry = fetchGeometry;

  if ( rect.isEmpty() )
  {
    mFetchRectangle = mExtent;
  }
  else
  {
    mFetchRectangle = rect;
  }

  mSelectedFeatures = mSpatialIndex->intersects( mFetchRectangle );
  rewind();
}

void QgsDarwinCoreProvider::rewind()
{
  mParseMode = Unknown;
  mAtEnd = false;
  mFinished = false; 
  mFeatureIterator = mSelectedFeatures.begin();
}

bool QgsDarwinCoreProvider::featureAtId( int featureId,QgsFeature& feature,
                                         bool fetchGeometry, QgsAttributeList fetchAttributes )
{
  QHash<int, QgsFeature* >::iterator it = mFeatures.find( featureId );
  if( it == mFeatures.constEnd() )
  {
    return false;
  }

  copyFeature( it.value(), feature, fetchGeometry );
  return true;
}

bool QgsDarwinCoreProvider::nextFeature( QgsFeature& feature )
{
  if ( mFeatures.size() == 0 || mFeatureIterator == mSelectedFeatures.end() )
  {
    return false;
  }

  copyFeature( mFeatures[*mFeatureIterator], feature, mFetchGeometry );
  ++mFeatureIterator;
  return true;
}

void QgsDarwinCoreProvider::copyFeature( QgsFeature* fet, QgsFeature& f, bool fetchGeometry )
{
  if( !fet )
  {
    return;
  }

  if( fetchGeometry )
  {
    QgsGeometry* geometry = fet->geometry();
    unsigned char *geom = geometry->asWkb();
    int geomSize = geometry->wkbSize();
    unsigned char* copiedGeom = new unsigned char[geomSize];
    memcpy( copiedGeom, geom, geomSize );
    f.setGeometryAndOwnership( copiedGeom, geomSize );
  }
  f.setAttributeMap( fet->attributeMap() );
  f.setFeatureId( fet->id() );
  f.setValid( true );
}

QGis::WkbType QgsDarwinCoreProvider::geometryType() const
{
  return QGis::WKBPoint;
}

long QgsDarwinCoreProvider::featureCount() const
{
  return mFeatures.size();
}

uint QgsDarwinCoreProvider::fieldCount() const
{
  return 0; //soon...
}

const QgsFieldMap& QgsDarwinCoreProvider::fields() const
{
  return mFieldMap;
}

int QgsDarwinCoreProvider::capabilities() const
{
  return SelectAtId;
}

QString QgsDarwinCoreProvider::name() const
{
  return TEXT_PROVIDER_KEY;
}

QString QgsDarwinCoreProvider::description() const
{
  return TEXT_PROVIDER_DESCRIPTION;
}

void QgsDarwinCoreProvider::startElement( const XML_Char* el, const XML_Char** attr )
{
  QString elementName( el );

  if( elementName == TAXON_NAMESPACE + NS_SEPARATOR + "TaxonOccurrence" )
  {
    mParseMode = TaxonOccurence;
  }


  if( mParseMode == TaxonOccurence )
  {
    mStringCash.clear();
  }
}

void QgsDarwinCoreProvider::endElement( const XML_Char* el )
{
  QString elementName( el );
  if( elementName == TAXON_NAMESPACE + NS_SEPARATOR + "decimalLongitude" )
  {
    mCurrentLongitude = mStringCash;
  }
  else if( elementName == TAXON_NAMESPACE + NS_SEPARATOR + "decimalLatitude" )
  {
    mCurrentLatitude = mStringCash;
  }
  else if( elementName == TAXON_NAMESPACE + NS_SEPARATOR + "TaxonOccurrence" )
  {
    bool lonConversionOk, latConversionOk;
    double lon = mCurrentLongitude.toDouble( &lonConversionOk );
    double lat = mCurrentLatitude.toDouble( &latConversionOk );

    int featureId = mFeatures.size();

    QgsFeature* taxonFeature = new QgsFeature( featureId );
    if( lonConversionOk &&  latConversionOk)
    {
      taxonFeature->setGeometry( QgsGeometry::fromPoint( QgsPoint( lon, lat ) ) );
    }
    taxonFeature->setAttributeMap( mCurrentAttributeMap );
    mCurrentAttributeMap.clear();

    //adjust bounding box
    if( mFeatures.size() < 1 )
    {
      mExtent = QgsRectangle(lon, lat, lon, lat);
    }
    else
    {
      mExtent. combineExtentWith( lon, lat );
    }

    mFeatures.insert( featureId, taxonFeature );
    mSpatialIndex->insertFeature( *taxonFeature );

    mParseMode = Unknown;

  }
  else if( mParseMode == TaxonOccurence )
  {
    mStringCash = mStringCash.trimmed();
    if( !mStringCash.isEmpty() ) //an attribute?
    {
      QString elName;
      QStringList elementNameSplit = elementName.split( NS_SEPARATOR );
      if( elementNameSplit.size() > 1 )
      {
        elName = elementNameSplit.at( 1 );
      }
      else
      {
        elName = elementNameSplit.at( 0 );
      }

      //check if already present
      int fieldIndex;
      QHash< QString, QPair<int, QgsField> >::const_iterator it = mCurrentFields.find( elName );
      if( it == mCurrentFields.constEnd() )
      {
        //insert new field
        fieldIndex = mCurrentFields.size();
        mCurrentFields.insert( elName, qMakePair( fieldIndex, QgsField( elName, QVariant::String, "String" ) ) );
      }
      else
      {
        fieldIndex = it.value().first;
      }
      mCurrentAttributeMap.insert( fieldIndex, QVariant(mStringCash) );
    }
  }
  mStringCash.clear();
}

void QgsDarwinCoreProvider::characters( const XML_Char* chars, int len )
{
  if( mParseMode == TaxonOccurence )
  {
    mStringCash.append( QString::fromUtf8( chars, len ) );
  }
}

QGISEXTERN QgsDarwinCoreProvider* classFactory( const QString *uri )
{
  return new QgsDarwinCoreProvider( *uri );
}

QGISEXTERN QString providerKey()
{
  return TEXT_PROVIDER_KEY;
}

QGISEXTERN QString description()
{
  return TEXT_PROVIDER_DESCRIPTION;
}

QGISEXTERN bool isProvider()
{
  return true;
}

