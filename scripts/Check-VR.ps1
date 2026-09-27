$ErrorActionPreference = 'Stop'
$runtimeKey = 'HKLM:\SOFTWARE\Khronos\OpenXR\1'
$runtime = (Get-ItemProperty -LiteralPath $runtimeKey -ErrorAction SilentlyContinue).ActiveRuntime
[pscustomobject]@{
    OpenXRRuntime = $runtime
    RuntimeFileExists = [bool]($runtime -and (Test-Path -LiteralPath $runtime))
    SteamVRServerRunning = [bool](Get-Process vrserver -ErrorAction SilentlyContinue)
    SteamVRCompositorRunning = [bool](Get-Process vrcompositor -ErrorAction SilentlyContinue)
}
Write-Output 'A running process does not prove that a headset is connected. Confirm the headset and controllers are green in SteamVR before starting Play-Game.ps1 -VR.'
