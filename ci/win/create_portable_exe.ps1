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
    $stubSourcePath = Join-Path $tmpRoot "launcher.cs"
    $references = @(
        (Get-FrameworkReference "System.IO.Compression.dll"),
        (Get-FrameworkReference "System.IO.Compression.FileSystem.dll")
    ) | Where-Object { $_ }

    Set-Content -Path $stubSourcePath -Value $stubSource -Encoding UTF8

    function Invoke-CSharpCompiler {
        param(
            [Parameter(Mandatory = $true)]
            [string]$SourcePath,
            [Parameter(Mandatory = $true)]
            [string]$OutputPath,
            [string[]]$References
        )

        $candidateCommands = @()

        $resolved = Get-Command "csc.exe" -ErrorAction SilentlyContinue
        if ($resolved) {
            $candidateCommands += $resolved.Source
        }

        $candidateCommands += @(
            (Join-Path $env:WINDIR "Microsoft.NET/Framework64/v4.0.30319/csc.exe"),
            (Join-Path $env:WINDIR "Microsoft.NET/Framework/v4.0.30319/csc.exe")
        ) | Where-Object { $_ }

        $compilerPath = $candidateCommands |
            Where-Object { $_ -and (Test-Path $_) } |
            Select-Object -First 1

        if (-not $compilerPath) {
            throw "Unable to locate csc.exe compiler on this system."
        }

        $referenceArgs = @()
        if ($References) {
            $referenceArgs = $References | ForEach-Object { "/r:`"$_`"" }
        }

        & $compilerPath "/nologo" "/target:exe" "/optimize+" "/out:$OutputPath" $referenceArgs $SourcePath
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path $OutputPath)) {
            throw "Failed to compile portable launcher using '$compilerPath' (exit code $LASTEXITCODE)."
        }
    }

    if (Test-Path $stubExe) { Remove-Item $stubExe -Force }

    Invoke-CSharpCompiler -SourcePath $stubSourcePath -OutputPath $stubExe -References $references

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
