$ErrorActionPreference = 'Stop'

$dir = Join-Path $PSScriptRoot 'models'
New-Item -ItemType Directory -Force -Path $dir | Out-Null

function Get-Model($url, $file, $sha256) {
    $path = Join-Path $dir $file

    if ((Test-Path $path) -and (Get-FileHash $path -Algorithm SHA256).Hash -eq $sha256) {
        Write-Host "$file is already there"
        return
    }

    Write-Host "Downloading $file"
    $part = "$path.part"
    if (Get-Command curl.exe -ErrorAction SilentlyContinue) {
        curl.exe -L --fail --progress-bar -o $part "$url/$file"
        if ($LASTEXITCODE -ne 0) { throw "Downloading $file failed" }
    } else {
        $ProgressPreference = 'SilentlyContinue'
        Invoke-WebRequest -Uri "$url/$file" -OutFile $part
    }

    if ((Get-FileHash $part -Algorithm SHA256).Hash -ne $sha256) {
        Remove-Item $part
        throw "$file does not match its SHA256, so it was deleted"
    }
    Move-Item -Force $part $path
}

Get-Model https://huggingface.co/mradermacher/GLM-OCR-GGUF/resolve/main `
    GLM-OCR.Q4_K_M.gguf bb27f2451f0f1b13491e04e0664cd946438a4677c32acf8ecfef79f68e862de1
Get-Model https://huggingface.co/ggml-org/GLM-OCR-GGUF/resolve/main `
    mmproj-GLM-OCR-Q8_0.gguf 9c4b58e33e316ed142eb5dcb41abec3844d3e6e5dc361ffb782c3fa9d175141f
