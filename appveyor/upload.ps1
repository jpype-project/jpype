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
    cd $env:APPVEYOR_BUILD_FOLDER
    $stylesheet =  "test/transform_xunit_to_appveyor.xsl"
    $input = "junit.xml"
    $output = "test/transformed.xml"
    
    xslt_transform $input $stylesheet $output

    upload $output
    Push-AppveyorArtifact $input
    Push-AppveyorArtifact $output
}

run
