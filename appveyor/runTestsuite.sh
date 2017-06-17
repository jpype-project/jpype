export PATH="/bin:/usr/bin"

if [ $PYTHON = "python3" ]; then
	NOSETESTS=nosetests-3.6
else
	NOSETESTS=nosetests-2.7
fi

$NOSETESTS --with-xunit --all-modules -w test/jpypetest
status=$?
echo "result code of nosetests:" $status 
exit $status
