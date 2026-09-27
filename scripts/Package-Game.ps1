param([string]$EngineRoot = 'E:\Epic Games\UE_5.8', [string]$OutputDirectory='')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $taskRoot 'CanyonRace\CanyonRace.uproject'
$output = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { Join-Path $taskRoot 'build' }
$uat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
& $uat BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$output" '-map=/Game/Canyon/Maps/L_Horseshoe' -unattended -utf8output
if ($LASTEXITCODE -ne 0) { throw "Packaging failed ($LASTEXITCODE)" }
Write-Output "Game packaged in $output\Windows"
