#TODO: Need to configure cmake to run pyrcc4 and pyuic4 as required when the resource
#      file or the ui change
SET(DRIVER_FILES

outputdrivers.db

)
INSTALL(FILES ${DRIVER_FILES} DESTINATION ${QGIS_DATA_DIR}/python/plugins/autogcp/drivers)
