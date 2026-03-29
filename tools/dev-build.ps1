param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Debug",

    [string]$BuildDir = "BuildsStandalone",
    [string]$Target = "SPOOL_Standalone",

    [switch]$Unity,
    [switch]$LTO,
    [switch]$NoCache,
    [switch]$Configure,
    [switch]$Run
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Resolve-Path (Join-Path $scriptDir "..")

Set-Location $repoRoot

$enableUnity = if ($Unity.IsPresent) { "ON" } else { "OFF" }
$enableLto = if ($LTO.IsPresent) { "ON" } else { "OFF" }
$enableCache = if ($NoCache.IsPresent) { "OFF" } else { "ON" }
$profileFlagsProvided = $PSBoundParameters.ContainsKey("Unity") `
    -or $PSBoundParameters.ContainsKey("LTO") `
    -or $PSBoundParameters.ContainsKey("NoCache")

function Invoke-Step {
    param(
        [string]$Title,
        [scriptblock]$Action
    )

    Write-Host "==> $Title" -ForegroundColor Cyan
    & $Action
}

$needConfigure = $Configure.IsPresent `
    -or $profileFlagsProvided `
    -or -not (Test-Path (Join-Path $BuildDir "CMakeCache.txt"))

if ($needConfigure)
{
    Invoke-Step "Configuring ($BuildDir, standalone-only)" {
        cmake -B $BuildDir `
            -DCMAKE_BUILD_TYPE=$Config `
            -DSPOOL_DEV_STANDALONE_ONLY=ON `
            -DSPOOL_ENABLE_COMPILER_CACHE=$enableCache `
            -DSPOOL_ENABLE_UNITY_BUILD=$enableUnity `
            -DSPOOL_ENABLE_LTO=$enableLto
    }
}

Write-Host ("Requested profile (applied on configure): cache={0} unity={1} lto={2}" -f $enableCache, $enableUnity, $enableLto) -ForegroundColor DarkGray

Invoke-Step "Building target '$Target' ($Config)" {
    cmake --build $BuildDir --config $Config --target $Target -- /m /p:TrackFileAccess=false
}

if ($Run)
{
    $exePath = Join-Path $repoRoot "$BuildDir/SPOOL_artefacts/$Config/Standalone/Spool.exe"
    if (Test-Path $exePath)
    {
        Invoke-Step "Launching standalone" {
            Start-Process -FilePath $exePath
        }
    }
    else
    {
        Write-Warning "Build succeeded, but executable not found at: $exePath"
    }
}

Write-Host "Done." -ForegroundColor Green
