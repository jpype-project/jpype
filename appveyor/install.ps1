# install apache ant
$ANT_VERSION = "1.9.6"

function RunCommand ($command, $command_args) {
    Write-Host $command $command_args
    Start-Process -FilePath $command -ArgumentList $command_args -Wait -Passthru
}


function unzipAnt($file, $destination) {
    if (-Not (Test-Path $file)) {
        Write-Host "File " $file "does not exist!"
        return
    }
    
    if (-Not (Test-Path $destination)) {
        mkdir $destination
    }

    Write-Host "extract " $file " to " $destination
    $shell_app = new-object -com shell.application
    $zip_file = $shell_app.namespace($file)
    $dir = $shell_app.namespace($destination)
    $dir.Copyhere($zip_file.items())

    # this should return the only folder name contained in ant.zip
    #$foldername = ""
    $zip_file.items() | foreach {
        $foldername = $_.name
        Write-Host "current foldername" $foldername
    }
    return $foldername
}

function DownloadAnt() {
    $url = "http://www.us.apache.org/dist/ant/binaries/apache-ant-$ANT_VERSION-bin.zip"
    $webclient = New-Object System.Net.WebClient
    $filepath = "C:\ant.zip"
	
	if (Test-Path $filepath) {
        Write-Host "Reusing" $filepath
        return $filepath
    }
	
    # Download and retry up to 3 times in case of network transient errors.
    Write-Host "Downloading" $filename "from" $url
    $retry_attempts = 2
    for($i=0; $i -lt $retry_attempts; $i++){
        try {
            $webclient.DownloadFile($url, $filepath)
            break
        }
        Catch [Exception]{
            Start-Sleep 1
        }
   }
   if (Test-Path $filepath) {
       Write-Host "File saved at" $filepath
   } else {
       # Retry once to get the error message if any at the last try
       $webclient.DownloadFile($url, $filepath)
   }
   return $filepath
}

function InstallAnt() {
    if (Test-Path "c:\apache-ant-$ANT_VERSION") {
        Write-Host "ant already exists"
        return $false
    }

    $filepath = DownloadAnt
    # extract to C: (will result in something like C:\apache-ant-1.9.4
    $folder = unzipAnt $filepath "C:"
    
    $ant_path  = "C:\" + $folder + "\bin"
    
    if (-not (Test-Path $ant_path)) {
        Throw "unpacked folder '" + $ant_path + "' does not exist!"
    }

    # create link to default ant binary dir, so we can rely on it.
    cmd.exe /c mklink /d C:\ant $folder
}

function main () {
    InstallAnt
}

main
