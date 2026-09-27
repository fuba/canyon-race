param([switch]$VR, [switch]$Capture, [string]$EngineRoot = 'E:\Epic Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot -Parent) 'CanyonRace\CanyonRace.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$playArgs = @(('"' + $project + '"'), '/Game/Canyon/Maps/L_Horseshoe', '-game', '-windowed', '-ResX=1440', '-ResY=900', '-NoSplash')
if ($VR) { $playArgs += '-vr' } else { $playArgs += '-nohmd' }
if ($Capture) { $playArgs += '-CanyonCaptureShot' }
Start-Process -FilePath $editor -ArgumentList $playArgs -WindowStyle Normal
