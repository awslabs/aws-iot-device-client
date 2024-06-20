# Capture parameters
$user = $args[0]
$fileUrl = $args[1]
$outputFile = $args[2]

$ErrorActionPreference = "Stop"

Write-Host ("Username (ignored in Windows): " + $user)
Write-Host ("File URL: $fileUrl")
Write-Host ("Write documents to file: $outputFile")

# Check target folder and create it if missing
$outputPath = Split-Path -Path $outputFile -Parent
if (-Not (Test-Path $outputPath)) {
    New-Item -Path $outputPath -ItemType Directory -Force | Out-Null
}

Invoke-WebRequest -Uri $fileUrl -OutFile $outputFile