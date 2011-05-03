#TODO: Need to configure cmake to run pyrcc4 and pyuic4 as required when the resource
#      file or the ui change
FILE(GLOB ICON_FILES *.png)
INSTALL(FILES ${ICON_FILES} DESTINATION ${QGIS_DATA_DIR}/python/plugins/autogcp/icons)
