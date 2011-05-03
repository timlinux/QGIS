/*! \ingroup analysis*/
/*! \brief A hash calculator used to generate hashes from strings and files.
 */
class QgsHasher
{
%TypeHeaderCode
#include <qgshasher.h>
%End
  public:

    /*! \brief Enumerator containing available options for the hash algorithm to be used.
     */
    enum HashAlgorithm
    {
      Md4,
      Md5,
      Sha1
    };

    /*! \brief Default constructor
    */
    QgsHasher();

    /*! \brief Calculates the hash value for the provided string based on the hash algorithm specified
     * \param data - The string to be converted to a hash value
     * \param hash - Specifies the hash algorithm to be used, defaults to MD5
     * \return A QString that represents the calculated hash value
     */
    QString getStringHash( QString data, HashAlgorithm hash = Md5 );

    /*! \brief Calculates the hash value for the provided file based on the hash algorithm specified
     * \param data - The path of the file to be used for the hash calculation.
     * \param hash - Specifies the hash algorithm to be used, defaults to MD5
     * \param numberOfBytes - Specifies the number of bytes to be read from the file, used for the hash calculation, defaults to -1 (read entire file)
     * \return A QString that represents the calculated hash value
     */
    QString getFileHash( QString data, HashAlgorithm hash = Md5, int numberOfBytes = -1 );
};
