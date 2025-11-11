param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,
    [Parameter(Mandatory = $true)]
    [string]$OutputExe,
    [string]$RunProgram = "QtScrcpy.exe",
    [string]$Title = "QtScrcpy Portable"
)

$ErrorActionPreference = "Stop"

function Resolve-FullPath([string]$path) {
    return [System.IO.Path]::GetFullPath($path)
}

$sourcePath = Resolve-Path -LiteralPath $SourceDir
$outputPath = Resolve-FullPath $OutputExe

if (-not (Test-Path $sourcePath)) {
    throw "Source directory '$SourceDir' does not exist."
}

function Find-SevenZip {
    $candidates = @(
        "${env:ProgramFiles}\7-Zip\7z.exe",
        "${env:ProgramFiles(x86)}\7-Zip\7z.exe"
    )
    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }
    throw "7-Zip executable not found. Please install 7-Zip on this machine."
}

$sevenZipExe = Find-SevenZip
$sevenZipDir = Split-Path $sevenZipExe -Parent

$tmpRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("qtscrcpy_sfx_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tmpRoot | Out-Null

try {
    $payloadArchive = Join-Path $tmpRoot "payload.7z"
    & $sevenZipExe a -t7z -mx=9 $payloadArchive "$sourcePath\*" | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "7-Zip failed to create payload archive."
    }

    $sfxModule = $null
    $localCandidates = @("7zSD.sfx", "7zS.sfx")
    foreach ($candidate in $localCandidates) {
        $modulePath = Join-Path $sevenZipDir $candidate
        if (Test-Path $modulePath) {
            $sfxModule = $modulePath
            break
        }
    }

    if (-not $sfxModule) {
        $moduleDir = Join-Path $tmpRoot "sfx"
        New-Item -ItemType Directory -Path $moduleDir | Out-Null
        $extraUrl = "https://www.7-zip.org/a/7z2301-extra.7z"
        $extraArchive = Join-Path $tmpRoot "7z-extra.7z"
        Write-Host "Downloading 7-Zip extra SFX module..."
        Invoke-WebRequest -Uri $extraUrl -OutFile $extraArchive
        & $sevenZipExe x -y "-o$moduleDir" $extraArchive "7zSD.sfx" | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to extract 7zSD.sfx from extra package."
        }
        $sfxModule = Join-Path $moduleDir "7zSD.sfx"
        if (-not (Test-Path $sfxModule)) {
            throw "7zSD.sfx not found after extraction."
        }
    }

    $useTempMode = (Split-Path $sfxModule -Leaf).ToLower().Contains("sd")

    $configPath = Join-Path $tmpRoot "config.txt"
    $configLines = @(
        ';!@Install@!UTF-8!'
        "Title=""$Title"""
    )
    if ($useTempMode) {
        $configLines += "TempMode"
    } else {
        $configLines += "ExtractTitle=""$Title"""
        $configLines += "GUIMode=""2"""
    }
    $configLines += "RunProgram=""$RunProgram"""
    $configLines += ';!@InstallEnd@!'
    $configLines | Set-Content -Encoding UTF8 $configPath

    $outputDir = Split-Path $outputPath -Parent
    if (-not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir | Out-Null
    }

    $destination = [System.IO.File]::Create($outputPath)
    foreach ($part in @($sfxModule, $configPath, $payloadArchive)) {
        $bytes = [System.IO.File]::ReadAllBytes($part)
        $destination.Write($bytes, 0, $bytes.Length)
    }
    $destination.Dispose()
    Write-Host "Portable executable created at $outputPath"
}
finally {
    if (Test-Path $tmpRoot) {
        Remove-Item $tmpRoot -Recurse -Force
    }
}
