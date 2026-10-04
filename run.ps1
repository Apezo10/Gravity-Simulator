param(
    [ValidateSet('release', 'local')]
    [string]$Preset = 'release'
)

# Use Release for everyday runs; local keeps the Debug build available.
Push-Location $PSScriptRoot
try {
    $configureArgs = @('--preset', $Preset)
    $buildDirectory = Join-Path $PSScriptRoot "build\$Preset"
    $cacheFile = Join-Path $buildDirectory 'CMakeCache.txt'

    if (Test-Path -LiteralPath $cacheFile) {
        $cachedPaths = @{}
        foreach ($line in Get-Content -LiteralPath $cacheFile) {
            if ($line -match '^CMAKE_(HOME_DIRECTORY|CACHEFILE_DIR):INTERNAL=(.*)$') {
                $cachedPaths[$Matches[1]] = $Matches[2].Replace('\', '/').TrimEnd('/')
            }
        }

        # CMake stores absolute paths. Refresh only when the cache belongs to
        # another source/build folder; ordinary launches keep incremental builds.
        $sourcePath = $PSScriptRoot.Replace('\', '/').TrimEnd('/')
        $buildPath = $buildDirectory.Replace('\', '/').TrimEnd('/')
        if ($cachedPaths['HOME_DIRECTORY'] -ine $sourcePath -or
            $cachedPaths['CACHEFILE_DIR'] -ine $buildPath) {
            Write-Host 'Project location changed or cache is incomplete; refreshing CMake configuration.'
            $configureArgs += '--fresh'
        }
    }

    cmake @configureArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & ".\build\$Preset\vectors.exe"
} finally {
    Pop-Location
}
