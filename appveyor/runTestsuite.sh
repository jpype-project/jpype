export PATH="/bin:/usr/bin:$PATH"
cd $APPVEYOR_BUILD_FOLDER

echo "==== Run test.jpypetest"
$PYTHON -m pytest -v --junitxml=junit.xml test/jpypetest

status=$?
echo "result code of pytest:" $status 

# Even if the pytest gave a 0, we better have a result to upload.
if [ ! -e junit.xml ]; then
	exit -1
fi

exit $status
