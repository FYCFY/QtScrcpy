param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,
    [Parameter(Mandatory = $true)]
    [string]$OutputExe,
    [string]$RunProgram = "QtScrcpy.exe",
    [string]$Title = "QtScrcpy Portable",
    [string]$RuntimeIdentifier
)

$ErrorActionPreference = "Stop"

function Resolve-FullPath([string]$path) {
    return [System.IO.Path]::GetFullPath((Resolve-Path -LiteralPath $path))
}

function Get-FrameworkReference([string]$assemblyName) {
    $root = Join-Path ${env:ProgramFiles(x86)} "Reference Assemblies/Microsoft/Framework/.NETFramework"
    if (Test-Path $root) {
        $version = Get-ChildItem $root -Directory | Sort-Object Name -Descending | Select-Object -First 1
        if ($version) {
            $candidate = Join-Path $version.FullName $assemblyName
            if (Test-Path $candidate) {
                return $candidate
            }
        }
    }
    return $assemblyName
}

$sourcePath = Resolve-FullPath -path $SourceDir
if (-not (Test-Path $sourcePath)) {
    throw "Source directory '$SourceDir' does not exist."
}
$outputPath = [System.IO.Path]::GetFullPath($OutputExe)
$outputDir = Split-Path $outputPath -Parent
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

$tmpRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("qtscrcpy_portable_" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tmpRoot | Out-Null

try {
    $payloadZip = Join-Path $tmpRoot "payload.zip"
    if (Test-Path $payloadZip) { Remove-Item $payloadZip -Force }
    Compress-Archive -Path (Join-Path $sourcePath "*") -DestinationPath $payloadZip -CompressionLevel Optimal

    $stubSource = @"
using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;

internal static class PortableLauncher
{
    private const string RunProgram = @"$RunProgram";
    private const string WindowTitle = @"$Title";
    private const int LengthMarkerSize = 8;

    private static int Main()
    {
        string exePath = System.Reflection.Assembly.GetExecutingAssembly().Location;
        long payloadLength = ReadPayloadLength(exePath);
        long payloadOffset = new FileInfo(exePath).Length - payloadLength - LengthMarkerSize;

        string tempRoot = Path.Combine(Path.GetTempPath(), "QtScrcpy_" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(tempRoot);
        string zipPath = Path.Combine(tempRoot, "payload.zip");

        try
        {
            CopyPayload(exePath, payloadOffset, payloadLength, zipPath);
            ZipFile.ExtractToDirectory(zipPath, tempRoot);
            File.Delete(zipPath);

            string targetExe = Path.Combine(tempRoot, RunProgram);
            if (!File.Exists(targetExe))
            {
                Console.Error.WriteLine($"[{WindowTitle}] Unable to locate '{RunProgram}' in extracted payload.");
                return 1;
            }

            var startInfo = new ProcessStartInfo(targetExe)
            {
                UseShellExecute = false,
                WorkingDirectory = Path.GetDirectoryName(targetExe)
            };
            var process = Process.Start(startInfo);
            if (process == null)
            {
                Console.Error.WriteLine($"[{WindowTitle}] Failed to start '{RunProgram}'.");
                return 1;
            }
            process.WaitForExit();
            return process.ExitCode;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"[{WindowTitle}] {ex}");
            return 1;
        }
        finally
        {
            try { if (Directory.Exists(tempRoot)) Directory.Delete(tempRoot, true); } catch { }
        }
    }

    private static long ReadPayloadLength(string exePath)
    {
        using (var fs = new FileStream(exePath, FileMode.Open, FileAccess.Read, FileShare.Read))
        {
            if (fs.Length < LengthMarkerSize)
            {
                throw new InvalidDataException("Portable package is corrupted (missing length).");
            }
            fs.Seek(-LengthMarkerSize, SeekOrigin.End);
            var lenBytes = new byte[LengthMarkerSize];
            if (fs.Read(lenBytes, 0, lenBytes.Length) != lenBytes.Length)
            {
                throw new InvalidDataException("Unable to read payload length.");
            }
            long length = BitConverter.ToInt64(lenBytes, 0);
            if (length <= 0 || length > fs.Length - LengthMarkerSize)
            {
                throw new InvalidDataException("Invalid payload length.");
            }
            return length;
        }
    }

    private static void CopyPayload(string exePath, long offset, long length, string destination)
    {
        const int bufferSize = 81920;
        var buffer = new byte[bufferSize];
        using (var input = new FileStream(exePath, FileMode.Open, FileAccess.Read, FileShare.Read))
        using (var output = new FileStream(destination, FileMode.Create, FileAccess.Write, FileShare.None))
        {
            input.Seek(offset, SeekOrigin.Begin);
            long remaining = length;
            while (remaining > 0)
            {
                int toRead = remaining > bufferSize ? bufferSize : (int)remaining;
                int read = input.Read(buffer, 0, toRead);
                if (read <= 0)
                {
                    throw new EndOfStreamException("Unexpected end of portable payload.");
                }
                output.Write(buffer, 0, read);
                remaining -= read;
            }
        }
    }
}
"@

    $stubExe = Join-Path $tmpRoot "launcher.exe"
    $references = @(
        (Get-FrameworkReference "System.IO.Compression.dll"),
        (Get-FrameworkReference "System.IO.Compression.FileSystem.dll")
    ) | Where-Object { $_ }

    if (Test-Path $stubExe) { Remove-Item $stubExe -Force }

    $isDesktopPwsh = $PSVersionTable.PSEdition -eq 'Desktop'
    if ($isDesktopPwsh) {
        Add-Type -TypeDefinition $stubSource `
            -Language CSharp `
            -OutputAssembly $stubExe `
            -OutputType ConsoleApplication `
            -ReferencedAssemblies $references `
            -CompilerOptions "/optimize+"
    }
    else {
        $stubSourcePath = Join-Path $tmpRoot "PortableLauncher.cs"
        Set-Content -Path $stubSourcePath -Value $stubSource -Encoding UTF8

        $projectPath = Join-Path $tmpRoot "PortableLauncher.csproj"
        $projectContent = @"
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <OutputType>Exe</OutputType>
    <TargetFramework>net6.0</TargetFramework>
    <ImplicitUsings>enable</ImplicitUsings>
    <Nullable>disable</Nullable>
    <AssemblyName>launcher</AssemblyName>
  </PropertyGroup>
</Project>
"@
        Set-Content -Path $projectPath -Value $projectContent -Encoding UTF8

        $publishDir = Join-Path $tmpRoot "publish"
        if (Test-Path $publishDir) { Remove-Item $publishDir -Recurse -Force }

        $dotnet = Get-Command dotnet -ErrorAction SilentlyContinue
        if (-not $dotnet) {
            throw "Unable to locate 'dotnet'. Ensure .NET SDK is installed on the build agent."
        }

        if (-not $RuntimeIdentifier) {
            $sourceDirLower = $SourceDir.ToLowerInvariant()
            if ($sourceDirLower -like "*\\x64\\*" -or $sourceDirLower -like "*/x64/*") {
                $RuntimeIdentifier = "win-x64"
            }
            elseif ($sourceDirLower -like "*\\arm64\\*" -or $sourceDirLower -like "*/arm64/*") {
                $RuntimeIdentifier = "win-arm64"
            }
            else {
                $RuntimeIdentifier = "win-x86"
            }
        }

        $publishArgs = @(
            "publish",
            $projectPath,
            "-c", "Release",
            "-o", $publishDir,
            "-r", $RuntimeIdentifier,
            "--self-contained", "true",
            "/p:PublishSingleFile=true",
            "/p:IncludeNativeLibrariesForSelfExtract=true",
            "/p:UseAppHost=true"
        )
        dotnet @publishArgs | Write-Host

        $publishedExe = Join-Path $publishDir "launcher.exe"
        if (-not (Test-Path $publishedExe)) {
            throw "dotnet publish did not produce launcher.exe"
        }

        Get-ChildItem -Path $publishDir | ForEach-Object {
            Copy-Item -Path $_.FullName -Destination $outputDir -Recurse -Force
        }

        $stubExe = $publishedExe
    }

    Copy-Item $stubExe $outputPath -Force

    $zipInfo = Get-Item $payloadZip
    $outStream = [System.IO.File]::Open($outputPath, [System.IO.FileMode]::Append, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
    try {
        $zipStream = [System.IO.File]::OpenRead($payloadZip)
        try {
            $zipStream.CopyTo($outStream)
        } finally {
            $zipStream.Dispose()
        }
        $lengthBytes = [System.BitConverter]::GetBytes([Int64]$zipInfo.Length)
        $outStream.Write($lengthBytes, 0, $lengthBytes.Length)
    } finally {
        $outStream.Dispose()
    }

    Write-Host "Portable executable created at $outputPath"
}
finally {
    if (Test-Path $tmpRoot) {
        Remove-Item $tmpRoot -Recurse -Force
    }
}
