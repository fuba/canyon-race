param([string]$BuildDirectory='')
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
$package = if ($BuildDirectory) { [IO.Path]::GetFullPath($BuildDirectory) } else { Join-Path $taskRoot 'build\Windows' }
$game = Join-Path $package 'CanyonRace\Binaries\Win64\CanyonRace.exe'
$saved = Join-Path $package 'CanyonRace\Saved'
foreach ($check in @(@('CanyonSmokeTest','CanyonSmoke.txt','river'),@('CanyonFlightTest','CanyonFlight.txt','river'),@('CanyonCourseTest','CanyonCourse.txt','river'),@('CanyonCourseTest','CanyonCourse.txt','summit'))) {
    $result = Join-Path $saved $check[1]
    if (Test-Path -LiteralPath $result) { Move-Item -LiteralPath $result -Destination "$result.previous" -Force }
    $checkArgs = @('-unattended','-nosound','-nohmd','-nullrhi','-UseFixedTimeStep','-FPS=60',('-'+$check[0]),('-CanyonCourse='+$check[2]))
    $testProcess = Start-Process -FilePath $game -ArgumentList $checkArgs -WorkingDirectory (Split-Path $game -Parent) -WindowStyle Hidden -Wait -PassThru
    if ($testProcess.ExitCode -ne 0) { throw "$($check[0]) exited $($testProcess.ExitCode)" }
    if (!(Test-Path -LiteralPath $result) -or !(Get-Content -LiteralPath $result -Raw).StartsWith('PASS:')) { throw "Missing successful $($check[0]) result at $result" }
    Get-Content -LiteralPath $result
}
