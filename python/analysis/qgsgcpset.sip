/*!\ingroup analysis*/
/*! \brief This is a container class that represents an entire set of
 * Gcps associated with a specific reference model or a specific extraction
 * and cross-referencing operation.
 */
class QgsGcpSet
{
%TypeHeaderCode
#include <qgsgcpset.h>
%End
  public:
    /*! \brief QgsGcpSet constructor
    Creates an empty Gcp set.
    */
    QgsGcpSet();
    /*! \brief QgsGcpSet destructor */
    virtual ~QgsGcpSet();
    /*! \brief Gets an ID associated with this ref Gcp dataset
     * Usually this is for database purposes, but can be used for other forms of unique identification.
     * \return Returns the ref ID of this Gcp dataset or -1 if no ID has been set.
     */
    int refId() const;
    /*! \brief Sets the ref ID of this Gcp dataset
     */
    void setRefId( int id );
    /*! \brief Gets an ID associated with this raw Gcp dataset
     * Usually this is for database purposes, but can be used for other forms of unique identification.
     * \return Returns the raw ID of this Gcp dataset or -1 if no ID has been set.
     */
    int rawId() const;
    /*! \brief Sets the raw ID of this Gcp dataset
     */
    void setRawId( int id );
    /*! \brief Adds a Gcp to this Gcp set.
     * May add duplicate entries.
     */
    void addGcp( QgsGcp* point );
    /*! \brief Updates a Gcp in this Gcp set.
     */
    void updateGcp( QgsGcp* oldPoint, QgsGcp* newPoint );
    /*! \brief  Removes a Gcp from the set.
     * This removes the first Gcp in the list with match reference x and y coordinates.
     */
    void removeGcp( QgsGcp* point );
    /*! \brief Gets the amount of Gcp's in the list
     */
    int size() const;
    /*! \brief Removes all Gcps from this Gcp set
     */
    void clear();
    /*! \brief  Returns the underlying QList
     */
    const QList<QgsGcp*>& constList() const;
    /*! \brief Returns a copy of the underlying QList
     */
    QList<QgsGcp*>& list();
};
