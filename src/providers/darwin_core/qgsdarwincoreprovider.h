#ifndef QGSDARWINCOREPROVIDER_H
#define QGSDARWINCOREPROVIDER_H

#include "qgsvectordataprovider.h"
#include <expat.h>
#include <QFile>

class QgsSpatialIndex;

/**A provider to read darwin core xml format*/
class QgsDarwinCoreProvider: public QgsVectorDataProvider
{
  public:
    QgsDarwinCoreProvider( const QString& uri );
    ~QgsDarwinCoreProvider();

    /** Select features based on a bounding rectangle. Features can be retrieved with calls to nextFeature.
     * @param fetchAttributes list of attributes which should be fetched
     * @param rect spatial filter
     * @param fetchGeometry true if the feature geometry should be fetched
     * @param useIntersect true if an accurate intersection test should be used,
     *                     false if a test based on bounding box is sufficient
     */
    virtual void select( QgsAttributeList fetchAttributes = QgsAttributeList(),
                         QgsRectangle rect = QgsRectangle(),
                         bool fetchGeometry = true,
                         bool useIntersect = false );

    /** Restart reading features from previous select operation */
    virtual void rewind();

    /**
     * Gets the feature at the given feature ID.
     * @param featureId of the feature to be returned
     * @param feature which will receive the data
     * @param fetchGeometry flag which if true, will cause the geometry to be fetched from the provider
     * @param fetchAttributes a list containing the indexes of the attribute fields to copy
     * @return True when feature was found, otherwise false
     *
     * Default implementation traverses all features until it finds the one with correct ID.
     * In case the provider supports reading the feature directly, override this function.
     */
    virtual bool featureAtId( int featureId,
                              QgsFeature& feature,
                              bool fetchGeometry = true,
                              QgsAttributeList fetchAttributes = QgsAttributeList() );

    /**
     * Get the next feature resulting from a select operation.
     * @param feature feature which will receive data from the provider
     * @return true when there was a feature to fetch, false when end was hit
     */
    virtual bool nextFeature( QgsFeature& feature );

    /**
     * Get feature type.
     * @return int representing the feature type
     */
    virtual QGis::WkbType geometryType() const;


    /**
     * Number of features in the layer
     * @return long containing number of features
     */
    virtual long featureCount() const;


    /**
     * Number of attribute fields for a feature in the layer
     */
    virtual uint fieldCount() const;

    /**
     * Return a map of indexes with field names for this layer
     * @return map of fields
     * @see QgsFieldMap
     */
    virtual const QgsFieldMap &fields() const;

    /**
     * Get the extent of the layer
     * @return QgsRectangle containing the extent of the layer
     */
    virtual QgsRectangle extent(){ return mExtent; }

    /*! Get the QgsCoordinateReferenceSystem for this layer
     * @note Must be reimplemented by each provider.
     * If the provider isn't capable of returning
     * its projection an empty srs will be return, ti will return 0
     */
    virtual QgsCoordinateReferenceSystem crs() { return mCrs; }

    virtual QString name() const;

    virtual QString description() const;

    /**
     * Returns true if this is a valid layer. It is up to individual providers
     * to determine what constitutes a valid layer
     */
    virtual bool isValid() { return mValid; }

    /** Returns a bitmask containing the supported capabilities
        Note, some capabilities may change depending on whether
        a spatial filter is active on this provider, so it may
        be prudent to check this value per intended operation.
     */
    virtual int capabilities() const;

  private:

    enum parseMode
    {
      //DecimalLongitude,
      //DecimalLatitude,
      TaxonOccurence,
      Unknown
    };

    parseMode mParseMode;

    const long int BUFFER_SIZE;

    char* mBuffer;

    QgsFieldMap mFieldMap;

    QgsRectangle mExtent;

    QgsCoordinateReferenceSystem mCrs;

    bool mValid;

    QFile mFile;

    /**Flag to cancel reading*/
    bool mFinished;

    /**True if end of file is reached*/
    bool mAtEnd;

    XML_Parser mXmlParser;

    /**Features (cached in memory)*/
    QHash<int, QgsFeature*> mFeatures;

    /**A spatial index for fast access to a feature subset*/
    QgsSpatialIndex *mSpatialIndex;

    /**Vector where the ids of the selected features are inserted*/
    QList<int> mSelectedFeatures;

    /**Iterator on the feature vector for use in rewind(), nextFeature(), etc...*/
    QList<int>::iterator mFeatureIterator;

    /**This contains the character data if an important element has been encountered*/
    QString mStringCash;

    QString mCurrentLatitude;
    QString mCurrentLongitude;

    /**Rectangle filter*/
    QgsRectangle mFetchRectangle;

    bool mFetchGeometry;

    /**Field map to check for already encountered attributes*/
    QHash< QString, QPair<int, QgsField> > mCurrentFields;

    /**Attribute map of the current feature*/
    QgsAttributeMap mCurrentAttributeMap;

    /**Reads the next part of the file (is done in steps of 10 * 1024 * 1024 bytes )*/
    void readNextPart();

    void deleteCachedFeatures();

    /**Copies feature attributes / geometry from fet to f*/
    void copyFeature( QgsFeature* fet, QgsFeature& f, bool fetchGeometry );

    /**File adresses of the features. The vector index of an entry is the feature id*/
    //QVector<qint64> mFeatureAdresses;

    /**XML handler methods*/
    void startElement( const XML_Char* el, const XML_Char** attr );
    void endElement( const XML_Char* el );
    void characters( const XML_Char* chars, int len );
    static void start( void* data, const XML_Char* el, const XML_Char** attr )
    {
      static_cast<QgsDarwinCoreProvider*>( data )->startElement( el, attr );
    }
    static void end( void* data, const XML_Char* el )
    {
      static_cast<QgsDarwinCoreProvider*>( data )->endElement( el );
    }
    static void chars( void* data, const XML_Char* chars, int len )
    {
      static_cast<QgsDarwinCoreProvider*>( data )->characters( chars, len );
    }
};

#endif // QGSDARWINCOREPROVIDER_H
