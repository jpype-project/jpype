cinst jdk8 -params "installdir=C:\\jdk8" %CINST_OPTS%
REM cinst jdk7 -params "installdir=C:\\jdk7"
REM cinst jdk9 -version 9.0.4.11 -params "installdir=C:\\jdk9"
cinst ant %CINST_OPTS%
REM Add thinges to path
SET PATH=%PYTHON%;%PYTHON%\\Scripts;%ANT_HOME%\\bin;%PATH%

%CYGSH% appveyor/install.sh

