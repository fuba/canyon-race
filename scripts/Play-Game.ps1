param([switch]$VR, [switch]$Quiet, [ValidateSet('river','summit')][string]$Course='river')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$game = Join-Path $taskRoot 'build\Windows\CanyonRace.exe'
if (!(Test-Path -LiteralPath $game)) { throw 'Packaged game not found. Run scripts/Package-Game.ps1 first.' }
$playArgs = @('-windowed','-ResX=1440','-ResY=900','-NoSplash')
$playArgs += "-CanyonCourse=$Course"
if ($VR) { $playArgs += '-vr' } else { $playArgs += '-nohmd' }
if ($Quiet) { $playArgs += '-nosound' }
Start-Process -FilePath $game -WorkingDirectory (Split-Path $game -Parent) -ArgumentList $playArgs -WindowStyle Normal
