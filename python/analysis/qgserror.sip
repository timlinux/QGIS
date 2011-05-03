class QgsError
{
%TypeHeaderCode
#include <qgserror.h>
%End
  public:
    QgsError();
    ~QgsError();
    void setMessage( QString message );

    void setClassOccured( QString classOccured );

    void setFunctionOccured( QString funcOccured );

    void setSeverity( QString severity );

    void setViewed( bool viewed );

    QString message();

    QString classOccured();

    QString functionOccured();

    QString severity();

    bool viewed();
};
