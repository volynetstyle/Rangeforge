$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$pixiRoot = Join-Path $root ".pixi"
$pixiBin = Join-Path $pixiRoot "bin"
$pixi = Join-Path $pixiBin "pixi.exe"
$version = "v0.81.0"
$url = "https://github.com/prefix-dev/pixi/releases/download/$version/pixi-x86_64-pc-windows-msvc.exe"
$expectedHash = "1b989b152ea13e881a641a112c7ea59c51818fd5ba84924c763b40b9b157dab9"

if (-not (Test-Path -LiteralPath $pixi)) {
    New-Item -ItemType Directory -Force -Path $pixiBin | Out-Null
    $temporaryPixi = "$pixi.download"
    try {
        Invoke-WebRequest -Uri $url -OutFile $temporaryPixi
        $actualHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $temporaryPixi).Hash.ToLowerInvariant()
        if ($actualHash -ne $expectedHash) {
            throw "Pixi checksum mismatch: expected $expectedHash, got $actualHash"
        }
        Move-Item -LiteralPath $temporaryPixi -Destination $pixi -Force
    }
    finally {
        Remove-Item -LiteralPath $temporaryPixi -Force -ErrorAction SilentlyContinue
    }
}

$env:PIXI_HOME = Join-Path $pixiRoot "home"
$env:PIXI_CACHE_DIR = Join-Path $pixiRoot "cache"
& $pixi install --locked
if ($LASTEXITCODE -ne 0) {
    throw "Pixi install failed with exit code $LASTEXITCODE"
}

Write-Host "Rangeforge is ready. Run: .\.pixi\bin\pixi.exe run check"
