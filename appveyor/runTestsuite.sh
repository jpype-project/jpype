export PATH="/bin:/usr/bin:$PATH"
cd $APPVEYOR_BUILD_FOLDER

if [ $PYTHON = "python3" ]; then
	NOSETESTS="nosetests-3.6"
else
	NOSETESTS="nosetests-2.7"
fi

echo "==== Run test.jpypetest"
$NOSETESTS -v --with-xunit --all-modules -s test.jpypetest

status=$?
echo "result code of nosetests:" $status 

exit $status
