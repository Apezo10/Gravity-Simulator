param(
    [ValidateSet('release', 'local')]
    [string]$Preset = 'release'
)

# Use Release for everyday runs; local keeps the Debug build available.
Push-Location $PSScriptRoot
try {
    $configureArgs = @('--preset', $Preset)
    $buildDirectory = Join-Path $PSScriptRoot "build\$Preset"
    $executable = Join-Path $buildDirectory 'vectors.exe'
    $cacheFile = Join-Path $buildDirectory 'CMakeCache.txt'
    $needsConfigure = $true

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
        } else {
            $needsConfigure = !(Test-Path -LiteralPath (Join-Path $buildDirectory 'build.ninja'))
            # Ninja tracks CMakeLists.txt, but preset changes require an explicit
            # configure to apply their new compiler/options to this build.
            if ((Get-Item -LiteralPath (Join-Path $PSScriptRoot 'CMakePresets.json')).LastWriteTimeUtc -gt
                (Get-Item -LiteralPath $cacheFile).LastWriteTimeUtc) {
                $needsConfigure = $true
            }
        }
    }

    if ($needsConfigure) {
        cmake @configureArgs
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    # Build the app and its dependencies; tests remain available via CMake.
    cmake --build --preset $Preset --target vectors
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $executable
} finally {
    Pop-Location
}
