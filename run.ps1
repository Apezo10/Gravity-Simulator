# Build through CMake so the compiler can find SFML's headers and libraries.
Push-Location $PSScriptRoot
try {
    cmake --preset local
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    cmake --build --preset local
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & .\build-ninja\vectors.exe
} finally {
    Pop-Location
}
