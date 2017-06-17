export PATH="/bin:/usr/bin:$PATH"
cd $APPVEYOR_BUILD_FOLDER

if [ $PYTHON = "python3" ]; then
	NOSETESTS="nosetests-3.6"
else
	NOSETESTS="nosetests-2.7"
fi

echo PATH=$PATH
echo APPVEYOR_BUILD_FOLDER="$APPVEYOR_BUILD_FOLDER"
echo NOSETESTS="$NOSETESTS"

$NOSETESTS --help

echo ===== "TRY 1"
$NOSETESTS -v --with-xunit --all-modules -w test/jpypetest

echo ===== "TRY 2"
$NOSETESTS -v --with-xunit --all-modules -s test/testsuite.py

status=$?
echo "result code of nosetests:" $status 

exit -1
#exit $status
