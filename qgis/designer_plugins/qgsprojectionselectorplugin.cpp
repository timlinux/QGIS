#include "qgsprojectionselectorplugin.h"
#include "../widgets/projectionselector/qgsprojectionselector.h"

static const char *projectionselector_pixmap[] = {
    "22 22 8 1",
    "  c Gray100",
    ". c Gray97",
    "X c #4f504f",
    "o c #00007f",
    "O c Gray0",
    "+ c none",
    "@ c Gray0",
    "# c Gray0",
    "++++++++++++++++++++++",
    "++++++++++++++++++++++",
    "++++++++++++++++++++++",
    "++++++++++++++++++++++",
    "+OOOOOOOOOOOOOOOOOOOO+",
    "OOXXXXXXXXXXXXXXXXXXOO",
    "OXX.          OO OO  O",
    "OX.      oo     O    O",
    "OX.      oo     O   .O",
    "OX  ooo  oooo   O    O",
    "OX    oo oo oo  O    O",
    "OX  oooo oo oo  O    O",
    "OX oo oo oo oo  O    O",
    "OX oo oo oo oo  O    O",
    "OX  oooo oooo   O    O",
    "OX            OO OO  O",
    "OO..................OO",
    "+OOOOOOOOOOOOOOOOOOOO+",
    "++++++++++++++++++++++",
    "++++++++++++++++++++++",
    "++++++++++++++++++++++",
    "++++++++++++++++++++++"
};

QgsProjectionSelectorPlugin::QgsProjectionSelectorPlugin()
{
}

QStringList QgsProjectionSelectorPlugin::keys() const
{
    QStringList list;
    list << "QgsProjectionSelector";
    return list;
}

QWidget* QgsProjectionSelectorPlugin::create( const QString &key, QWidget* parent, const char* name )
{
    if ( key == "QgsProjectionSelector" )
	return new QgsProjectionSelector( parent, name );
    return 0;
}

QString QgsProjectionSelectorPlugin::group( const QString& feature ) const
{
    if ( feature == "QgsProjectionSelector" )
	return "Input";
    return QString::null;
}

QIconSet QgsProjectionSelectorPlugin::iconSet( const QString& ) const
{
    return QIconSet( QPixmap( projectionselector_pixmap ) );
}

QString QgsProjectionSelectorPlugin::includeFile( const QString& feature ) const
{
    if ( feature == "QgsProjectionSelector" )
	return "qgsprojectionselector.h";
    return QString::null;
}

QString QgsProjectionSelectorPlugin::toolTip( const QString& feature ) const
{
    if ( feature == "QgsProjectionSelector" )
	return "QGIS Projection Selector Widget";
    return QString::null;
}

QString QgsProjectionSelectorPlugin::whatsThis( const QString& feature ) const
{
    if ( feature == "QgsProjectionSelector" )
	return "A widget to choose a projection name from a list";
    return QString::null;
}

bool QgsProjectionSelectorPlugin::isContainer( const QString& ) const
{
    return FALSE;
}


Q_EXPORT_PLUGIN( QgsProjectionSelectorPlugin )
