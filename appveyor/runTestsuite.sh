export PATH="/bin:/usr/bin"
nosetests test/jpypetest --all-modules --with-xunit
$success = $?
echo "result code of nosetests:" $success
