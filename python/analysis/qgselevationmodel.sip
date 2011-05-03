/*! \ingroup analysis*/
/*! \brief An elevation model.
 * The elevation model is used to retrieved height/ elevation values for specified points from a Digital Elevation Model.
 */

class QgsElevationModel
{
%TypeHeaderCode
#include <qgselevationmodel.h>
%End
  public:
    /*! \brief Creates a Elevation Model Dataset
    * \param filename The path to the DEM file.
    */
    QgsElevationModel( QString filename );
    /*! \brief Determine the elevation of the specified geographic point
    * \param geoX The geographic X coordinate
    * \param geoY The geographic Y coordinate
    * \param geoZ The geographic Z coordinate that will be determined
    */
    bool elevateGeographicPoint( const double &geoX, const double &geoY, double &geoZ );
    /*! \brief Determine the elevation of the specified pixel point
    * \param pixelX The pixel X coordinate
    * \param pixelY The pixel Y coordinate
    * \param geoZ The geographic Z coordinate that will be determined
    */
    bool elevatePixelPoint( const int pixelX, const int pixelY, double &geoZ );
    /*! \brief Determines the highest elevation value in the DEM
    */
    double highestElevationValue();
    /*! \brief Determines the lowest elevation value in the DEM
    */
    double lowestElevationValue();
    /*! \brief Constant value for DEM band number
    */
};
