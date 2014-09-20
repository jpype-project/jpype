function xslt_transform($xml, $xsl, $output)
{
	trap [Exception]
	{
	    Write-Host $_.Exception
	}
	
	$xslt = New-Object System.Xml.Xsl.XslCompiledTransform
	$xslt.Load($xsl)
	$xslt.Transform($xml, $output)
}

function upload($file) {
    trap [Exception]
    {
        Write-Host $_.Exception
    }

    $wc = New-Object 'System.Net.WebClient'
    $wc.UploadFile("https://ci.appveyor.com/api/testresults/xunit/$($env:APPVEYOR_JOB_ID)", $file)
}

function run {
    $stylesheet = $env:APPVEYOR_BUILD_FOLDER + "/test/transform_xunit_to_appveyor.xsl""
    $output = $env:APPVEYOR_BUILD_FOLDER + "/test/transformed.xml"
    
    nosetests test/jpypetest --all-modules --with-xunit
    $success = $?
    Write-Host "result code of nosetests:" $success
    
    $input = $env:APPVEYOR_BUILD_FOLDER + "/nosetests.xml"

    xslt_transform $input $stylesheet $output

    upload $output
    
    
    Push-AppveyorArtifact $input
    Push-AppveyorArtifact $output
    
    
    # return exit code of testsuite
    if ( -not $success) {
        throw "testsuite not successful"
    }
}

run