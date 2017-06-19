cd $env:APPVEYOR_BUILD_FOLDER
nosetests test/jpypetest --all-modules --with-xunit
$success = $?
Write-Host "result code of nosetests:" $success

# return exit code of testsuite
if ( -not $success) {
   throw "testsuite not successful"
}
