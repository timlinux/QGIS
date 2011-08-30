FILE(GLOB PY_FILES *.py)

SET(INSTALLER_FILES ${PY_FILES})

ADD_CUSTOM_TARGET(sumbandilasat_structurer ALL DEPENDS )
INSTALL(FILES ${INSTALLER_FILES} DESTINATION ${QGIS_DATA_DIR}/python/plugins/sumbandilasat/tools/structurer)