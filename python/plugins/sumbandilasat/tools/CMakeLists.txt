FILE(GLOB INSTALLER_FILES *.py)
SET(INSTALLER_FILES ${INSTALLER_FILES})

INSTALL(FILES ${INSTALLER_FILES} DESTINATION ${QGIS_DATA_DIR}/python/plugins/sumbandilasat/tools)

SUBDIRS(
georeferencer
columncorrector
cloudcoverer
bandaligner
linecorrector
structurer
)