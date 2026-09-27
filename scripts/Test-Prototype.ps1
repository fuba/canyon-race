param([string]$EngineRoot = 'E:\Epic Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$project = Join-Path $taskRoot 'CanyonRace\CanyonRace.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$report = Join-Path $taskRoot 'artifacts\automation'
& $editor $project -unattended -nop4 -nosound -nullrhi -nohmd -nosplash '-ExecCmds=Automation RunTests Canyon.Rules' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$report" '-log=RulesTests.log'
if ($LASTEXITCODE -ne 0) { throw "Rule tests failed ($LASTEXITCODE)" }
$summary=Get-Content (Join-Path $report 'index.json') -Raw | ConvertFrom-Json
if ($summary.failed -ne 0 -or $summary.succeeded -lt 8) { throw 'Rule report did not confirm all tests passed.' }
$smoke = Join-Path $taskRoot 'CanyonRace\Saved\CanyonSmoke.txt'
if (Test-Path -LiteralPath $smoke) { Move-Item -LiteralPath $smoke -Destination "$smoke.previous" -Force }
& $editor $project /Game/Canyon/Maps/L_Horseshoe -game -unattended -nop4 -nosound -nullrhi -nohmd -nosplash -UseFixedTimeStep '-FPS=60' -CanyonSmokeTest '-log=SmokeTest.log'
if ($LASTEXITCODE -ne 0) { throw "World smoke test failed ($LASTEXITCODE)" }
if (!(Test-Path -LiteralPath $smoke) -or !(Get-Content -LiteralPath $smoke -Raw).StartsWith('PASS:')) { throw 'World smoke test did not finish successfully' }
Get-Content -LiteralPath $smoke
