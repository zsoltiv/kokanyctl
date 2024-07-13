$PORTS = "1338", "1341"

if ( $Args.Count -ne 1 ) {
    Write-Error "Usage [index]"
    Exit
}

$cameras = (ffmpeg -list_devices true -f dshow -i dummy 2>&1 | Select-String '"@device_pnp_.*"').Matches.Value.Replace('"', '').Split([Environment]::NewLine)
$i = [int]($Args[0])
$port = $PORTS[$i]
$camera = $cameras[$i]

Write-Host "Az index " $i
Write-Host "A port " $port
Write-Host "A kamera " $camera

if ( $cameras.Count -lt 2 ) {
    Write-Error "Nem minden kamera van bekotve"
    Exit
} elseif ( $cameras.Count -gt 2 ) {
    Write-Error "Harom kamerat latok, kapcsold ki a beepitettet"
    Exit
}

$output = "udp://127.0.0.1:" + $port
Write-Host "A kimenet pedig " $output

ffmpeg -f dshow -fflags nobuffer -framerate 30 -rtbufsize 1024M -i video=$camera -c:v copy -f mjpeg -fflags nobuffer $output
