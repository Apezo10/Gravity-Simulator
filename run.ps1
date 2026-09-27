param(
    [ValidateSet('release', 'local')]
    [string]$Preset = 'release'
)

# Use Release for everyday runs; local keeps the Debug build available.
Push-Location $PSScriptRoot
try {
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & ".\build\$Preset\vectors.exe"
} finally {
    Pop-Location
}
