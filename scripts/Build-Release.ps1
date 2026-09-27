param([string]$EngineRoot='E:\Epic Games\UE_5.8', [string]$PreparedCheckout='')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot -Parent
Push-Location $taskRoot
try {
    $branch=git branch --show-current
    if ($LASTEXITCODE -ne 0 -or $branch -ne 'main') { throw 'Build a release from main.' }
    if (git status --porcelain --untracked-files=no) { throw 'Commit tracked changes before releasing.' }
    $commit=git rev-parse HEAD
    $version=(Get-Content VERSION -Raw).Trim()
    if ($version -notmatch '^\d+\.\d+\.\d+$') { throw 'VERSION must be major.minor.patch.' }
    $releaseRoot=Join-Path $taskRoot ('artifacts\release-'+$version+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
    $checkout=if ($PreparedCheckout) { [IO.Path]::GetFullPath($PreparedCheckout) } else { Join-Path $releaseRoot 'source' }
    New-Item -ItemType Directory -Path $releaseRoot -Force | Out-Null
    # A fresh checkout proves that ignored templates, local assets, and private logs are unnecessary.
    if ($PreparedCheckout) {
        $allowed=([IO.Path]::GetFullPath((Join-Path $taskRoot 'artifacts'))+[IO.Path]::DirectorySeparatorChar)
        if (!$checkout.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)) { throw 'Resume only a release checkout under this project artifacts directory.' }
        $origin=git -C $checkout remote get-url origin
        if ($LASTEXITCODE -ne 0 -or [IO.Path]::GetFullPath($origin) -ne [IO.Path]::GetFullPath($taskRoot)) { throw 'Checkout must originate from this project.' }
        if (git -C $checkout status --porcelain --untracked-files=all) { throw 'Prepared checkout has changes.' }
        git -C $checkout fetch origin main
        if ($LASTEXITCODE -ne 0) { throw 'Failed to fetch main.' }
        git -C $checkout merge --ff-only FETCH_HEAD
        if ($LASTEXITCODE -ne 0) { throw 'Prepared checkout cannot fast-forward to main.' }
    } else {
        git clone --no-local --branch main $taskRoot $checkout
        if ($LASTEXITCODE -ne 0) { throw 'Clean source clone failed.' }
    }
    if ((git -C $checkout rev-parse HEAD) -ne $commit) { throw 'Source commit changed during clone.' }
    & (Join-Path $checkout 'scripts\Build-Prototype.ps1') -EngineRoot $EngineRoot
    & (Join-Path $checkout 'scripts\Test-Prototype.ps1') -EngineRoot $EngineRoot
    & (Join-Path $checkout 'scripts\Package-Game.ps1') -EngineRoot $EngineRoot
    $package=Join-Path $checkout 'build\Windows'
    & (Join-Path $checkout 'scripts\Test-Packaged.ps1') -BuildDirectory $package
    if (!(Test-Path -LiteralPath (Join-Path $package 'Engine\Extras\Redist\en-us\vc_redist.x64.exe'))) { throw 'Missing Microsoft Visual C++ prerequisite installer.' }
    $payload=Join-Path $releaseRoot 'CanyonRace-Windows'
    New-Item -ItemType Directory -Path $payload | Out-Null
    foreach ($file in Get-ChildItem -LiteralPath $package -File -Recurse) {
        $relative=$file.FullName.Substring($package.Length+1)
        if ($relative -match '(^|[\\/])(Saved|Intermediate)([\\/]|$)' -or $file.Extension -eq '.pdb' -or $file.Name -like 'Manifest_*') { continue }
        $target=Join-Path $payload $relative
        New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $target
    }
    foreach ($name in @('LICENSE.md','THIRD_PARTY_NOTICES.md')) { Copy-Item -LiteralPath (Join-Path $checkout $name) -Destination $payload }
    Copy-Item -LiteralPath (Join-Path $checkout 'docs\playing.md') -Destination (Join-Path $payload 'README.md')
    foreach ($name in @('Play-VR.cmd','Play-Desktop.cmd','Play-Summit-VR.cmd','Play-Summit-Desktop.cmd')) {
        Copy-Item -LiteralPath (Join-Path $checkout ('scripts\launchers\'+$name)) -Destination $payload
    }
    $metadata=[ordered]@{version=$version;commit=$commit;branch='main';engine='5.8.3';configuration='Development';platform='Win64';built_utc=(Get-Date).ToUniversalTime().ToString('o');checks=@('Canyon.Rules','World smoke','Aerodynamic controls','River course','Summit course')}
    $metadata | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $payload 'build-info.json') -Encoding utf8
    $output=Join-Path $taskRoot 'release'
    New-Item -ItemType Directory -Path $output -Force | Out-Null
    $zip=Join-Path $output "canyon-race-v$version-windows-x64.zip"
    if (Test-Path -LiteralPath $zip) { throw "Refusing to overwrite existing artifact: $zip" }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [IO.Compression.ZipFile]::CreateFromDirectory($payload,$zip,[IO.Compression.CompressionLevel]::Optimal,$false)
    $hash=(Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $([IO.Path]::GetFileName($zip))" | Set-Content -LiteralPath "$zip.sha256" -Encoding ascii
    Copy-Item -LiteralPath (Join-Path $payload 'build-info.json') -Destination (Join-Path $output 'build-info.json')
    Write-Output "Release artifact: $zip"
    Write-Output "Source: $commit"
    Write-Output "Clean checkout: $checkout"
} finally { Pop-Location }
