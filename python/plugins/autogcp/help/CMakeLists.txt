#TODO: Need to configure cmake to run pyrcc4 and pyuic4 as required when the resource
#      file or the ui change
SET(HELP_FILES

DatabaseDialog
ImageInfoDialog
RectificationDialog
SettingsDialog

)
INSTALL(FILES ${HELP_FILES} DESTINATION ${QGIS_DATA_DIR}/python/plugins/autogcp/help)
