cd $env:APPVEYOR_BUILD_FOLDER
python.exe -m pytest -v --junitxml=junit.xml test/jpypetest --checkjni
$success = $?
Write-Host "result code of pytest:" $success

# return exit code of testsuite
if ( -not $success) {
   throw "testsuite not successful"
}
