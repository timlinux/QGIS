/*! \ingroup analysis*/
/*! \brief A Ground Control Point (Gcp) abstraction contains information about
* a specific Gcp such as the ground and associated image coordinates of this
* Gcp, as well as additional elevation and other information. This object is
* also the container for the Gcp chip data.
*/
class QgsGcp
{
%TypeHeaderCode
#include <qgsgcp.h>
%End
  public:
    QgsGcp();
    
    ~QgsGcp();
    /*! \brief Gets an ID associated with this Gcp
     * Usually this is for database purposes, but can be used for other forms of unique identification.
     * \return Returns the ID of this Gcp or -1 if no ID has been set.
     */
    int id() const;
    /*! \brief Sets the ID of this Gcp
     */
    void setId( int id );
    /*! \brief Returns the X-component of this Gcp's geographic coordinates.
     */
    double refX() const;
    /*! \brief Sets the X-component of this Gcp's geographic coordinates.
     */
    void setRefX( double value );
    /*! \brief Returns the Y-component of this Gcp's geographic coordinates.
     */
    double refY() const;
    /*! \brief Sets the Y-component of this Gcp's geographic coordinates.
     */
    void setRefY( double value );
    /*! \brief Returns the Z-component of this Gcp's geographic coordinates.
     * This is usually either 0 or the height as loaded from a Digital Elevation Model
     */
    double refZ() const;
    /*! \brief Sets the Z-component of this Gcp's geographic coordinates.
     */
    void setRefZ( double value );
    /*! \brief Gets the pixel column of this point on the associated image
     */
    double rawX() const;
    /*! \brief Sets the pixel column of this point on the associated image
     */
    void setRawX( double value );
    /*! \brief Gets the pixel line of this point on the associated image
     */
    double rawY() const;
    /*! \brief Sets the pixel line of this point on the associated image
     */
    void setRawY( double value );
    double correlationCoefficient() const;
    void setCorrelationCoefficient( double value );
    /*!\brief Gets the Gcp chip
     * The returned pointer must not be deleted while the Gcp is still being used.
     * \return Returns a pointer to the chip as a QgsRasterDataset, or NULL if no chip has been extracted.
     */
    QgsRasterDataset* gcpChip() const;
    /*!\brief Sets the Gcp chip
     */
    void setGcpChip( QgsRasterDataset* value );
protected:
  QgsGcp( const QgsGcp& other );
};
