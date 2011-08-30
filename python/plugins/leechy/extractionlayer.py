# -*- coding: utf-8 -*-
import os.path

class ExtractionLayer():

  def __init__(self):
    self.url = "file:///" + os.path.dirname( __file__ ).replace("\\", "/") + "/OpenLayers.js"
  
  def get(self, extent, name):
    if name == "OpenStreetMap":
      return self.getOpenStreetMap(extent)
    elif name == "GooglePhysical":
      return self.getGooglePhysical(extent)
    elif name == "GoogleStreets":
      return self.getGoogleStreets(extent)
    elif name == "GoogleHybrid":
      return self.getGoogleHybrid(extent)
    elif name == "GoogleSatellite":
      return self.getGoogleSatellite(extent)
    elif name == "YahooStreet":
      return self.getYahooStreet(extent)
    elif name == "YahooHybrid":
      return self.getYahooHybrid(extent)
    elif name == "YahooSatellite":
      return self.getYahooSatellite(extent)
  
  def getOpenStreetMap(self, extent):
    return """ 
      <html xmlns="http://www.w3.org/1999/xhtml">
	<head>
	  <title>OpenLayers OpenStreetMap Layer</title>
	  <style type="text/css">
	      body {
		  margin: 0;
	      }
	      #map {
		  width: 100%;
		  height: 100%;
	      }
	      .olControlAttribution {
		  font-size: smaller;
		  right: -50px;
		  bottom: 0.5em;
		  position: absolute;
		  display: block;
	      }
	  </style>
	  <script src='"""+self.url+"""'></script>
	  <script type="text/javascript">
	      var map;
	      var loadEnd;

	      function init() {
		  map = new OpenLayers.Map('map', {
		      theme: null,
		      controls: [],
		      projection: new OpenLayers.Projection("EPSG:900913"),
		      units: "m",
		      maxResolution: 156543.0339,
		      maxExtent: new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34)
		  });

		  loadEnd = false;
		  function layerLoadStart(event)
		  {
		    loadEnd = false;
		  }
		  function layerLoadEnd(event)
		  {
		    loadEnd = true;
		  }

		  var osm = new OpenLayers.Layer.OSM(
		    "OpenStreetMap",
		    "http://tile.openstreetmap.org/${z}/${x}/${y}.png",
		    {
		      eventListeners: {
			"loadstart": layerLoadStart,
			"loadend": layerLoadEnd
		      }
		    }
		  );
		  map.addLayer(osm);
		  map.addControl(new OpenLayers.Control.Attribution());
		  map.setCenter(new OpenLayers.LonLat(0, 0), 2);
		  map.zoomToExtent(new OpenLayers.Bounds("""+str(extent.xMinimum())+""", """+str(extent.yMinimum())+""", """+str(extent.xMaximum())+""", """+str(extent.yMaximum())+"""));
	      }
	  </script>
	</head>
	<body onload="init()">
	  <div id="map"></div>
	</body>
      </html>
    """
    
  def getGooglePhysical(self, extent):
    return """ 
      <html xmlns="http://www.w3.org/1999/xhtml">
	<head>
	  <title>OpenLayers Google Physical Layer</title>
	  <style type="text/css">
	      body {
		  margin: 0;
	      }
	      #map {
		  width: 100%;
		  height: 100%;
	      }
	      .olLayerGoogleCopyright {
		  left: -50px;
		  bottom: -50px;
	      }
	      .olLayerGooglePoweredBy {
		  left: -50px;
		  bottom: -50px;
	      }
	      .olControlAttribution {
		  font-size: smaller;
		  right: 0px;
		  bottom: 4.5em;
		  position: absolute;
		  display: block;
	      }
	  </style>
	  <!-- this gmaps key generated for http://openlayers.org/dev/ -->
	  <script src='http://maps.google.com/maps?file=api&amp;v=2&amp;key=ABQIAAAAjpkAC9ePGem0lIq5XcMiuhR_wWLPFku8Ix9i2SXYRVK3e45q1BQUd_beF8dtzKET_EteAjPdGDwqpQ'></script>
	  <script src='"""+self.url+"""'></script>
	  <script type="text/javascript">
	      var map;

	      function init() {
		  map = new OpenLayers.Map('map', {
		      theme: null,
		      controls: [],
		      projection: new OpenLayers.Projection("EPSG:900913"),
		      units: "m",
		      maxResolution: 156543.0339,
		      maxExtent: new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34)
		  });

		  var gmap = new OpenLayers.Layer.Google(
		      "Google Physical",
		      {type: G_PHYSICAL_MAP, numZoomLevels: 16, 'sphericalMercator': true}
		  );
		  map.addLayer(gmap);
		  map.setCenter(new OpenLayers.LonLat(0, 0), 2);
		  map.zoomToExtent(new OpenLayers.Bounds("""+str(extent.xMinimum())+""", """+str(extent.yMinimum())+""", """+str(extent.xMaximum())+""", """+str(extent.yMaximum())+"""));
	      }
	  </script>
	</head>
	<body onload="init()">
	  <div id="map"></div>
	</body>
      </html>
    """

  def getGoogleStreets(self, extent):
    return """ 
      <html xmlns="http://www.w3.org/1999/xhtml">
	<head>
	  <title>OpenLayers Google Streets Layer</title>
	  <style type="text/css">
	      body {
		  margin: 0;
	      }
	      #map {
		  width: 100%;
		  height: 100%;
	      }
	      .olLayerGoogleCopyright {
		  left: -50px;
		  bottom: -50px;
	      }
	      .olLayerGooglePoweredBy {
		  left: -50px;
		  bottom: -50px;
	      }
	      .olControlAttribution {
		  font-size: smaller;
		  right: 0px;
		  bottom: 4.5em;
		  position: absolute;
		  display: block;
	      }
	  </style>
	  <!-- this gmaps key generated for http://openlayers.org/dev/ -->
	  <script src='http://maps.google.com/maps?file=api&amp;v=2&amp;key=ABQIAAAAjpkAC9ePGem0lIq5XcMiuhR_wWLPFku8Ix9i2SXYRVK3e45q1BQUd_beF8dtzKET_EteAjPdGDwqpQ'></script>
	  <script src='"""+self.url+"""'></script>
	  <script type="text/javascript">
	      var map;

	      function init() {
		  map = new OpenLayers.Map('map', {
		      theme: null,
		      controls: [],
		      projection: new OpenLayers.Projection("EPSG:900913"),
		      units: "m",
		      maxResolution: 156543.0339,
		      maxExtent: new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34)
		  });

		  var gmap = new OpenLayers.Layer.Google(
		      "Google Streets", // the default
		      {numZoomLevels: 20, 'sphericalMercator': true}
		  );
		  map.addLayer(gmap);
		  map.setCenter(new OpenLayers.LonLat(0, 0), 2);
		  map.zoomToExtent(new OpenLayers.Bounds("""+str(extent.xMinimum())+""", """+str(extent.yMinimum())+""", """+str(extent.xMaximum())+""", """+str(extent.yMaximum())+"""));
	      }
	  </script>
	</head>
	<body onload="init()">
	  <div id="map"></div>
	</body>
      </html>
    """

  def getGoogleHybrid(self, extent):
    return """ 
      <html xmlns="http://www.w3.org/1999/xhtml">
	<head>
	  <title>OpenLayers Google Hybrid Layer</title>
	  <style type="text/css">
	      body {
		  margin: 0;
	      }
	      #map {
		  width: 100%;
		  height: 100%;
	      }
	      .olLayerGoogleCopyright {
		  left: -50px;
		  bottom: -50px;
	      }
	      .olLayerGooglePoweredBy {
		  left: -50px;
		  bottom: -50px;
	      }
	      .olControlAttribution {
		  font-size: smaller;
		  right: 0px;
		  bottom: 4.5em;
		  position: absolute;
		  display: block;
	      }
	  </style>
	  <!-- this gmaps key generated for http://openlayers.org/dev/ -->
	  <script src='http://maps.google.com/maps?file=api&amp;v=2&amp;key=ABQIAAAAjpkAC9ePGem0lIq5XcMiuhR_wWLPFku8Ix9i2SXYRVK3e45q1BQUd_beF8dtzKET_EteAjPdGDwqpQ'></script>
	  <script src='"""+self.url+"""'></script>
	  <script type="text/javascript">
	      var map;

	      function init() {
		  map = new OpenLayers.Map('map', {
		      theme: null,
		      controls: [],
		      projection: new OpenLayers.Projection("EPSG:900913"),
		      units: "m",
		      maxResolution: 156543.0339,
		      maxExtent: new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34)
		  });

		  var gmap = new OpenLayers.Layer.Google(
		      "Google Hybrid",
		      {type: G_HYBRID_MAP, numZoomLevels: 20, 'sphericalMercator': true}
		  );
		  map.addLayer(gmap);
		  map.setCenter(new OpenLayers.LonLat(0, 0), 2);
		  map.zoomToExtent(new OpenLayers.Bounds("""+str(extent.xMinimum())+""", """+str(extent.yMinimum())+""", """+str(extent.xMaximum())+""", """+str(extent.yMaximum())+"""));
	      }
	  </script>
	</head>
	<body onload="init()">
	  <div id="map"></div>
	</body>
      </html>
    """

  def getGoogleSatellite(self, extent):
    return """ 
      <html xmlns="http://www.w3.org/1999/xhtml">
	<head>
	  <title>OpenLayers Google Satellite Layer</title>
	  <style type="text/css">
	      body {
		  margin: 0;
	      }
	      #map {
		  width: 100%;
		  height: 100%;
	      }
	      .olLayerGoogleCopyright {
		  left: -50px;
		  bottom: -50px;
	      }
	      .olLayerGooglePoweredBy {
		  left: -50px;
		  bottom: -50px;
	      }
	      .olControlAttribution {
		  font-size: smaller;
		  right: 0px;
		  bottom: 4.5em;
		  position: absolute;
		  display: block;
	      }
	  </style>
	  <!-- this gmaps key generated for http://openlayers.org/dev/ -->
	  <script src='http://maps.google.com/maps?file=api&amp;v=2&amp;key=ABQIAAAAjpkAC9ePGem0lIq5XcMiuhR_wWLPFku8Ix9i2SXYRVK3e45q1BQUd_beF8dtzKET_EteAjPdGDwqpQ'></script>
	  <script src='"""+self.url+"""'></script>
	  <script type="text/javascript">
	      var map;

	      function init() {
		  map = new OpenLayers.Map('map', {
		      theme: null,
		      controls: [],
		      projection: new OpenLayers.Projection("EPSG:900913"),
		      units: "m",
		      maxResolution: 156543.0339,
		      maxExtent: new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34)
		  });

		  var gmap = new OpenLayers.Layer.Google(
		      "Google Satellite",
		      {type: G_SATELLITE_MAP, numZoomLevels: 20, 'sphericalMercator': true}
		  );
		  map.addLayer(gmap);
		  map.setCenter(new OpenLayers.LonLat(0, 0), 2);
		  map.zoomToExtent(new OpenLayers.Bounds("""+str(extent.xMinimum())+""", """+str(extent.yMinimum())+""", """+str(extent.xMaximum())+""", """+str(extent.yMaximum())+"""));
	      }

	  </script>
	</head>
	<body onload="init()">
	  <div id="map"></div>
	</body>
      </html>
    """

  def getYahooStreet(self, extent):
    return """ 
      <html xmlns="http://www.w3.org/1999/xhtml">
	<head>
	  <title>OpenLayers Yahoo Street Layer</title>
	  <style type="text/css">
	      body {
		  margin: 0;
	      }
	      #map {
		  width: 100%;
		  height: 100%;
	      }
	  </style>
	  <script src="http://api.maps.yahoo.com/ajaxymap?v=3.0&appid=euzuro-openlayers"></script>
	  <script src='"""+self.url+"""'></script>
	  <script type="text/javascript">
	      var map;

	      function init() {
		  map = new OpenLayers.Map('map', {
		      theme: null,
		      controls: [],
		      projection: new OpenLayers.Projection("EPSG:900913"),
		      units: "m",
		      maxResolution: 156543.0339,
		      maxExtent: new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34)
		  });

		  var yahoo = new OpenLayers.Layer.Yahoo(
		      "Yahoo Street",
		      {'sphericalMercator': true}
		  );
		  map.addLayer(yahoo);
		  map.setCenter(new OpenLayers.LonLat(0, 0), 2);
		  map.zoomToExtent(new OpenLayers.Bounds("""+str(extent.xMinimum())+""", """+str(extent.yMinimum())+""", """+str(extent.xMaximum())+""", """+str(extent.yMaximum())+"""));
	      }
	  </script>
	</head>
	<body onload="init()">
	  <div id="map"></div>
	</body>
      </html>
    """

  def getYahooHybrid(self, extent):
    return """ 
      <html xmlns="http://www.w3.org/1999/xhtml">
	<head>
	  <title>OpenLayers Yahoo Hybrid Layer</title>
	  <style type="text/css">
	      body {
		  margin: 0;
	      }
	      #map {
		  width: 100%;
		  height: 100%;
	      }
	  </style>
	  <script src="http://api.maps.yahoo.com/ajaxymap?v=3.0&appid=euzuro-openlayers"></script>
	  <script src='"""+self.url+"""'></script>
	  <script type="text/javascript">
	      var map;

	      function init() {
		  map = new OpenLayers.Map('map', {
		      theme: null,
		      controls: [],
		      projection: new OpenLayers.Projection("EPSG:900913"),
		      units: "m",
		      maxResolution: 156543.0339,
		      maxExtent: new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34)
		  });

		  var yahoo = new OpenLayers.Layer.Yahoo(
		      "Yahoo Hybrid",
		      {'type': YAHOO_MAP_HYB, 'sphericalMercator': true}
		  );
		  map.addLayer(yahoo);
		  map.setCenter(new OpenLayers.LonLat(0, 0), 2);
		  map.zoomToExtent(new OpenLayers.Bounds("""+str(extent.xMinimum())+""", """+str(extent.yMinimum())+""", """+str(extent.xMaximum())+""", """+str(extent.yMaximum())+"""));
	      }
	  </script>
	</head>
	<body onload="init()">
	  <div id="map"></div>
	</body>
      </html>
    """
    
  def getYahooSatellite(self, extent):
    return """ 
      <html xmlns="http://www.w3.org/1999/xhtml">
	<head>
	  <title>OpenLayers Yahoo Satellite Layer</title>
	  <style type="text/css">
	      body {
		  margin: 0;
	      }
	      #map {
		  width: 100%;
		  height: 100%;
	      }
	  </style>
	  <script src="http://api.maps.yahoo.com/ajaxymap?v=3.0&appid=euzuro-openlayers"></script>
	  <script src='"""+self.url+"""'></script>
	  <script type="text/javascript">
	      var map;

	      function init() {
		  map = new OpenLayers.Map('map', {
		      theme: null,
		      controls: [],
		      projection: new OpenLayers.Projection("EPSG:900913"),
		      units: "m",
		      maxResolution: 156543.0339,
		      maxExtent: new OpenLayers.Bounds(-20037508.34, -20037508.34, 20037508.34, 20037508.34)
		  });

		  var yahoo = new OpenLayers.Layer.Yahoo(
		      "Yahoo Satellite",
		      {'type': YAHOO_MAP_SAT, 'sphericalMercator': true}
		  );
		  map.addLayer(yahoo);
		  map.setCenter(new OpenLayers.LonLat(0, 0), 2);
		  map.zoomToExtent(new OpenLayers.Bounds("""+str(extent.xMinimum())+""", """+str(extent.yMinimum())+""", """+str(extent.xMaximum())+""", """+str(extent.yMaximum())+"""));
	      }
	  </script>
	</head>
	<body onload="init()">
	  <div id="map"></div>
	</body>
      </html>
    """

