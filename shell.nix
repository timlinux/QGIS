let
  pinnedHash = "933d7dc155096e7575d207be6fb7792bc9f34f6d"; 
  pinnedPkgs = import (fetchTarball "https://github.com/NixOS/nixpkgs/archive/${pinnedHash}.tar.gz") { };
  py = pinnedPkgs.python3Packages;
  pyOveride = pinnedPkgs.python3Packages.override {
    packageOverrides = self: super: {
      pyqt5 = super.pyqt5.override {
        withLocation = true;
        withSerialPort = true;
      };
    };
  };
in pinnedPkgs.mkShell rec {
  name = "impurePythonEnv";
  venvDir = "./.venv";
  buildInputs = [
    # A Python interpreter including the 'venv' module is required to bootstrap
    # the environment.
    py.python
    # This executes some shell code to initialize a venv in $venvDir before
    py.venvShellHook
    py.requests
    pinnedPkgs.git

    # This executes some shell code to initialize a venv in $venvDir before
    # dropping into the shell
    py.venvShellHook
    py.virtualenv
    py.chardet
    py.debugpy
    py.future
    py.gdal
    py.jinja2
    py.matplotlib
    py.numpy
    py.owslib
    py.pandas
    py.plotly
    py.psycopg2
    py.pygments
    py.pyqt5
    py.pyqt5_with_qtwebkit # Added by Tim for InaSAFE
    py.pyqt-builder
    py.pyqtgraph # Added by Tim for QGIS Animation workbench (should probably be standard)
    py.python-dateutil
    py.pytz
    py.pyyaml
    py.qscintilla-qt5
    py.requests
    py.setuptools
    py.sip
    py.six
    py.sqlalchemy # Added by Tim for QGIS Animation workbench
    py.urllib3

    pinnedPkgs.makeWrapper
    pinnedPkgs.wrapGAppsHook
    #pinnedPkgs.wrapQtAppsHook

    pinnedPkgs.gcc
    pinnedPkgs.cmake
    pinnedPkgs.cmakeWithGui
    pinnedPkgs.flex
    pinnedPkgs.bison
    pinnedPkgs.ninja

    pinnedPkgs.draco
    pinnedPkgs.exiv2
    pinnedPkgs.fcgi
    pinnedPkgs.geos
    pinnedPkgs.gsl
    pinnedPkgs.hdf5
    pinnedPkgs.libspatialindex
    pinnedPkgs.libspatialite
    pinnedPkgs.libzip
    pinnedPkgs.netcdf
    pinnedPkgs.openssl
    pinnedPkgs.pdal
    pinnedPkgs.postgresql
    pinnedPkgs.proj
    pinnedPkgs.protobuf
    pinnedPkgs.libsForQt5.qca-qt5
    pinnedPkgs.qscintilla
    pinnedPkgs.libsForQt5.qt3d
    pinnedPkgs.libsForQt5.qtbase
    pinnedPkgs.libsForQt5.qtkeychain
    pinnedPkgs.libsForQt5.qtlocation
    pinnedPkgs.libsForQt5.qtmultimedia
    pinnedPkgs.libsForQt5.qtsensors
    pinnedPkgs.libsForQt5.qtserialport
    pinnedPkgs.libsForQt5.qtwebkit
    pinnedPkgs.libsForQt5.qtxmlpatterns
    pinnedPkgs.libsForQt5.qwt
    pinnedPkgs.saga # Probably not needed for build
    pinnedPkgs.sqlite
    pinnedPkgs.txt2tags
    pinnedPkgs.zstd
  ];
  #
  # ⚠️  Note: Do not use shellHook here too - it will prevent postVenvCreation 
  #          from running. Also use the ./clean script if you change the 
  #          postVenvCreation procedure below.
  #

  # Run this command, only after creating the virtual environment
  postVenvCreation = ''
     unset SOURCE_DATE_EPOCH
     echo "🐍 Installing Python Dependencies."
     pip install -r requirements.txt
     echo "🧊 Verifying python packages with pip freeze"
     pip freeze 
  '';

  # Now we can execute any commands within the virtual environment.
  # This is optional and can be left out to run pip manually.
  postShellHook = ''
    # allow pip to install wheels
    unset SOURCE_DATE_EPOCH
  '';
}

