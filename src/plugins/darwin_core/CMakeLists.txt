
########################################################
# Files

SET (DARWIN_CORE_SRCS
     qgsdarwincoreplugin.cpp
     qgsdarwincoredialog.cpp
)

SET (DARWIN_CORE_UIS qgsdarwincoredialogbase.ui)

SET (DARWIN_CORE_MOC_HDRS
     qgsdarwincoreplugin.h
     qgsdarwincoredialog.h
)

SET (DARWIN_CORE_RCCS  darwincore.qrc)

########################################################
# Build

QT4_WRAP_UI (DARWIN_CORE_UIS_H  ${DARWIN_CORE_UIS})

QT4_WRAP_CPP (DARWIN_CORE_MOC_SRCS  ${DARWIN_CORE_MOC_HDRS})

QT4_ADD_RESOURCES(DARWIN_CORE_RCC_SRCS ${DARWIN_CORE_RCCS})

ADD_LIBRARY (darwincoreplugin MODULE ${DARWIN_CORE_SRCS} ${DARWIN_CORE_MOC_SRCS} ${DARWIN_CORE_UIS_H} ${DARWIN_CORE_RCC_SRCS})

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     ${CMAKE_CURRENT_BINARY_DIR}/../../ui
     ../../core ../../core/raster ../../core/renderer ../../core/symbology
     ../../gui
     ..
     ${GEOS_INCLUDE_DIR}
)

TARGET_LINK_LIBRARIES(darwincoreplugin
  qgis_core
  qgis_gui
)


########################################################
# Install

INSTALL(TARGETS darwincoreplugin
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR})
