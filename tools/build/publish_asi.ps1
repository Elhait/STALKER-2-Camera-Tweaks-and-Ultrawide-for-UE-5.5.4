param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Destination
)

$mutex = $null
$ownsMutex = $false
$temporaryPath = $null

try {
    $sourcePath = [System.IO.Path]::GetFullPath($Source)
    $destinationPath = [System.IO.Path]::GetFullPath($Destination)
    if (-not [System.IO.File]::Exists($sourcePath)) {
        throw "Built ASI does not exist: $sourcePath"
    }

    $destinationDirectory = [System.IO.Path]::GetDirectoryName($destinationPath)
    [System.IO.Directory]::CreateDirectory($destinationDirectory) | Out-Null

    $normalizedDestination = $destinationPath.ToUpperInvariant()
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $pathBytes = [System.Text.Encoding]::UTF8.GetBytes($normalizedDestination)
        $nameHash = [System.BitConverter]::ToString($sha.ComputeHash($pathBytes)).Replace('-', '')
    }
    finally {
        $sha.Dispose()
    }

    $mutex = [System.Threading.Mutex]::new($false, "Local\STALKER2CameraTweaks.Publish.$nameHash")
    try {
        $ownsMutex = $mutex.WaitOne(0)
    }
    catch [System.Threading.AbandonedMutexException] {
        $ownsMutex = $true
    }
    if (-not $ownsMutex) {
        throw "Another build is publishing to this destination: $destinationPath"
    }

    if ([System.IO.File]::Exists($destinationPath) -and
        (Get-Item -LiteralPath $sourcePath).Length -eq (Get-Item -LiteralPath $destinationPath).Length) {
        $sha = [System.Security.Cryptography.SHA256]::Create()
        try {
            $sourceStream = [System.IO.File]::OpenRead($sourcePath)
            $destinationStream = [System.IO.File]::OpenRead($destinationPath)
            try {
                $sourceHash = [System.BitConverter]::ToString($sha.ComputeHash($sourceStream))
                $destinationHash = [System.BitConverter]::ToString($sha.ComputeHash($destinationStream))
            }
            finally {
                $sourceStream.Dispose()
                $destinationStream.Dispose()
            }
        }
        finally {
            $sha.Dispose()
        }
        if ($sourceHash -eq $destinationHash) {
            Write-Output "ASI is already current: $destinationPath"
            exit 0
        }
    }

    $destinationName = [System.IO.Path]::GetFileName($destinationPath)
    $temporaryPath = [System.IO.Path]::Combine(
        $destinationDirectory,
        ".${destinationName}.$([guid]::NewGuid().ToString('N')).tmp")
    [System.IO.File]::Copy($sourcePath, $temporaryPath, $false)

    if (-not ('CameraTweaks.AtomicPublish' -as [type])) {
        Add-Type -TypeDefinition @'
using System.Runtime.InteropServices;
namespace CameraTweaks {
    public static class AtomicPublish {
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool MoveFileEx(string existingFileName, string newFileName, uint flags);
    }
}
'@
    }
    if (-not [CameraTweaks.AtomicPublish]::MoveFileEx($temporaryPath, $destinationPath, 0x9)) {
        $win32Error = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw [ComponentModel.Win32Exception]::new($win32Error)
    }

    Write-Output "Published $destinationPath"
    exit 0
}
catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
finally {
    if ($temporaryPath -and [System.IO.File]::Exists($temporaryPath)) {
        Remove-Item -LiteralPath $temporaryPath -Force -ErrorAction SilentlyContinue
    }
    if ($ownsMutex -and $mutex) {
        $mutex.ReleaseMutex()
    }
    if ($mutex) {
        $mutex.Dispose()
    }
}
