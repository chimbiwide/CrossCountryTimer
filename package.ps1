$ErrorActionPreference = 'Stop'

function Invoke-Checked {
    $exe, $rest = $args
    & $exe @rest
    if ($LASTEXITCODE -ne 0) { throw "$args failed" }
}

Push-Location $PSScriptRoot
try {
    Invoke-Checked cmake --preset windows
    Invoke-Checked cmake --build --preset windows -j $env:NUMBER_OF_PROCESSORS
    Invoke-Checked cmake -S roster_parser -B roster_parser/build -G 'MinGW Makefiles' -DCMAKE_BUILD_TYPE=Release
    Invoke-Checked cmake --build roster_parser/build -j $env:NUMBER_OF_PROCESSORS

    $out = Join-Path $PSScriptRoot 'dist\CrossCountryTimer'
    New-Item -ItemType Directory -Force -Path "$out\data", "$out\models" | Out-Null
    Copy-Item -Force -Destination $out -Path `
        build\CrossCountryTimer.exe, roster_parser\build\RosterParser.exe, roster_parser\build\pdfium.dll
    Copy-Item -Force -Destination "$out\models" -Path `
        models\GLM-OCR.Q4_K_M.gguf, models\mmproj-GLM-OCR-Q8_0.gguf
    # the roster the timer reads. The font and the whisper model are inside the exe.
    if (Test-Path data\index.xlsx) { Copy-Item -Force data\index.xlsx "$out\data" }

    Write-Host "Done: copy $out to the other PC"
}
finally {
    Pop-Location
}
