export PATH="/bin:/usr/bin"
nosetests --with-xunit --all-modules -w test/jpypetest
status=$?
echo "result code of nosetests:" $status 
exit $status
