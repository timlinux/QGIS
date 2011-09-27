
########################################################
# Files

SET (DARWIN_CORE_SRCS qgsdarwincoreprovider.cpp)

SET (DARWIN_CORE_MOC_HDRS
	qgsdarwincoreprovider.h
)

########################################################
# Build

INCLUDE_DIRECTORIES(
  ../../core
  ../../core/spatialindex
  ${GEOS_INCLUDE_DIR}
  ${EXPAT_INCLUDE_DIR}
)

QT4_WRAP_CPP(DARWIN_CORE_MOC_SRCS ${DARWIN_CORE_MOC_HDRS})

ADD_LIBRARY(darwincoreprovider MODULE ${DARWIN_CORE_SRCS} ${DARWIN_CORE_MOC_SRCS})

TARGET_LINK_LIBRARIES(darwincoreprovider
  ${EXPAT_LIBRARY}
  qgis_core
)


########################################################
# Install

INSTALL (TARGETS darwincoreprovider
  RUNTIME DESTINATION ${QGIS_PLUGIN_DIR}
  LIBRARY DESTINATION ${QGIS_PLUGIN_DIR}) 
