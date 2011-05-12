

#############################################################
# sources

SET(QGIS_ANALYSIS_SRCS
  interpolation/qgsgridfilewriter.cpp
  interpolation/qgsidwinterpolator.cpp
  interpolation/qgsinterpolator.cpp
  interpolation/qgstininterpolator.cpp
  interpolation/Bezier3D.cc
  interpolation/CloughTocherInterpolator.cc
  interpolation/DualEdgeTriangulation.cc
  interpolation/HalfEdge.cc
  interpolation/Line3D.cc
  interpolation/LinTriangleInterpolator.cc
  interpolation/MathUtils.cc
  interpolation/NormVecDecorator.cc
  interpolation/Node.cc
  interpolation/ParametricLine.cc
  interpolation/Point3D.cc
  interpolation/TriangleInterpolator.cc
  interpolation/Triangulation.cc
  interpolation/TriDecorator.cc
  interpolation/Vector3D.cc
  raster/qgsninecellfilter.cpp
  raster/qgsruggednessfilter.cpp
  raster/qgsderivativefilter.cpp
  raster/qgsslopefilter.cpp
  raster/qgsaspectfilter.cpp
  raster/qgstotalcurvaturefilter.cpp
  raster/qgsrastercalcnode.cpp
  raster/qgsrastercalculator.cpp
  raster/qgsrastermatrix.cpp
  vector/qgsgeometryanalyzer.cpp
  vector/qgszonalstatistics.cpp
  vector/qgsoverlayanalyzer.cpp
  autogcp/qgsgcp.cpp
  autogcp/qgsrasterdataset.cpp
  autogcp/qgsgcpset.cpp
  autogcp/qgsautogcpmanager.cpp
  autogcp/qgsdatasource.cpp
  autogcp/qgsimageanalyzer.cpp
  autogcp/qgsbilinearsampler.cpp
  autogcp/qgsimagefilter.cpp
  autogcp/qgsmatrix.cpp
  autogcp/qgsorthorectificationfilter.cpp
  autogcp/qgsgeoreferencer.cpp
  autogcp/qgsrpcmodel.cpp
  autogcp/qgswavelettransform.cpp
  autogcp/qgsimagechip.cpp
  autogcp/qgsprojectionmanager.cpp
  autogcp/qgscrosscorrelator.cpp
  autogcp/qgshasher.cpp
  autogcp/qgssqldriver.cpp
  autogcp/qgssqlitedriver.cpp
  autogcp/qgselevationmodel.cpp
  autogcp/qgsgcptransform.cpp
  autogcp/qgstpstransform.cpp
  autogcp/qgscoordinatesystemtransform.cpp
)

INCLUDE_DIRECTORIES(BEFORE raster)
ADD_FLEX_FILES_PREFIX(QGIS_ANALYSIS_SRCS raster raster/qgsrastercalclexer.ll)

ADD_BISON_FILES_PREFIX(QGIS_ANALYSIS_SRCS raster raster/qgsrastercalcparser.yy)

IF (MSVC AND PEDANTIC)
# disable warnings
# 4065 switch with default and no case
# 4702 unreachable code
SET_SOURCE_FILES_PROPERTIES(
                ${CMAKE_BINARY_DIR}/src/analysis/qgsrastercalcparser.cpp 
                PROPERTIES COMPILE_FLAGS "-wd4065 -wd4702")
ENDIF (MSVC AND PEDANTIC)

SET(QGIS_ANALYSIS_MOC_HDRS
)

QT4_WRAP_CPP(QGIS_ANALYSIS_MOC_SRCS ${QGIS_ANALYSIS_MOC_HDRS})


INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/renderer
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/spatialindex
  ${CMAKE_CURRENT_SOURCE_DIR}/../core/raster
  interpolation
  autogcp
  ${PROJ_INCLUDE_DIR}
  ${GEOS_INCLUDE_DIR}
  ${GDAL_INCLUDE_DIR}
  ${GSL_INCLUDE_DIR}
)

IF (WIN32)
  IF (MSVC)
    ADD_DEFINITIONS("-DANALYSIS_EXPORT=__declspec(dllexport)")
  ELSE (MSVC)
    ADD_DEFINITIONS("-UANALYSIS_EXPORT \"-DANALYSIS_EXPORT=__declspec(dllexport)\"")
  ENDIF (MSVC)
ENDIF (WIN32)


#############################################################
# qgis_analysis library

ADD_LIBRARY(qgis_analysis SHARED ${QGIS_ANALYSIS_SRCS} ${QGIS_ANALYSIS_MOC_SRCS} )

ADD_DEPENDENCIES(qgis_analysis qgis_core)

SET_TARGET_PROPERTIES(qgis_analysis PROPERTIES VERSION ${COMPLETE_VERSION} SOVERSION ${COMPLETE_VERSION})

TARGET_LINK_LIBRARIES(qgis_analysis
  qgis_core
  ${GSL_LIBRARIES}
)

IF (APPLE)
  SET_TARGET_PROPERTIES(qgis_analysis PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE )
ENDIF (APPLE)

INSTALL(TARGETS qgis_analysis
  RUNTIME DESTINATION ${QGIS_BIN_DIR}
  LIBRARY DESTINATION ${QGIS_LIB_DIR}
  ARCHIVE DESTINATION ${QGIS_LIB_DIR})


# Added by Tim to install headers

SET(QGIS_ANALYSIS_HDRS vector/qgsgeometryanalyzer.h vector/qgszonalstatistics.h vector/qgsgeometryanalyzer.h interpolation/qgsinterpolator.h interpolation/qgsgridfilewriter.h interpolation/qgsidwinterpolator.h interpolation/qgstininterpolator.h
autogcp/qgsautogcpmanager.h
autogcp/qgsdatasource.h
autogcp/qgselevationmodel.h
autogcp/qgsgcp.h
autogcp/qgsgcpset.h
autogcp/qgsimageanalyzer.h
autogcp/qgsrasterdataset.h
autogcp/qgssampler.h
autogcp/qgsbilinearsampler.h
autogcp/qgsmatrix.cpp
autogcp/qgsimagefilter.h
autogcp/qgsorthorectificationfilter.h
autogcp/qgsgeoreferencer.h
autogcp/qgssensormodel.h
autogcp/qgsrpcmodel.h
autogcp/qgssalientpoint.h
autogcp/qgsimagechip.h
autogcp/qgscrosscorrelator.h
autogcp/qgsprojectionmanager.h
autogcp/qgshasher.h
autogcp/qgssqldriver.h
autogcp/qgssqlitedriver.h
autogcp/qgswavelettransform.h
autogcp/qgsgcptransform.h
autogcp/qgstpstransform.h
autogcp/qgscoordinatesystemtransform.h
)

INSTALL(CODE "MESSAGE(\"Installing ANALYSIS headers...\")")
INSTALL(FILES ${QGIS_ANALYSIS_HDRS} ${QGIS_ANALYSIS_MOC_HDRS} DESTINATION ${QGIS_INCLUDE_DIR}) 

