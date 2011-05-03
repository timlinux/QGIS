/*! \ingroup analysis*/
/*! \brief Container class for handling errors that occurs within the AutoGCP library
*/
class QgsErrorList
{
%TypeHeaderCode
#include <qgserrorlist.h>
%End
  public:
    /*! \brief Returns the valid instance
     * Returns a pointer to an existing instance, or creates a new one if no other instance exists.
    */
    static QgsErrorList* instance();
    /*! \brief Report an error
     * Reports an error to the main error list.
     * \param The message describing the error
     * \param The class name where the error occured
     * \param The function name where the error occured
     * \param The severity description of the error
     * \param True if the error should be printed to QgsLogger::debug()
    */
    void reportError( QString message, QString classOccured, QString functionOccured, QString severity, bool printDebug );
    /*! \brief Returns the last entered error
     * Returns the last submitted error in QString format
    */
    QString lastError();
    /*! \brief Deletes the last error
    */
    void deleteLastError();
    /*! \brief Composes a message
     * Composes a QString formatted message based on the provided QgsError
    */
    QString composeMessage( QgsError err );
  protected:
    QgsErrorList();
    QgsErrorList( const QgsErrorList &errList );
};
