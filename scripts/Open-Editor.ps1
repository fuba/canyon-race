param([string]$EngineRoot = 'E:\Epic Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot -Parent) 'CanyonRace\CanyonRace.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
if (!(Test-Path -LiteralPath $editor)) { throw "Unreal Editor not found: $editor" }
if (!(Test-Path -LiteralPath $project)) { throw "Project not found: $project" }
Start-Process -FilePath $editor -ArgumentList ('"' + $project + '"')
