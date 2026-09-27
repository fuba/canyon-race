param([string]$EngineRoot = 'E:\Epic Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot -Parent) 'CanyonRace\CanyonRace.uproject'
& (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') CanyonRaceEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw "Unreal build failed ($LASTEXITCODE)" }
