param(
    [Parameter(Mandatory = $true)][string]$ExecutablePath,
    [Parameter(Mandatory = $true)][string]$ZipPath,
    [Parameter(Mandatory = $true)][ValidatePattern('^[a-fA-F0-9]{64}$')][string]$ExpectedExeSha256,
    [Parameter(Mandatory = $true)][ValidatePattern('^[a-fA-F0-9]{64}$')][string]$ExpectedZipSha256
)
$ErrorActionPreference = 'Stop'
if ((Get-FileHash -LiteralPath $ExecutablePath -Algorithm SHA256).Hash -ne $ExpectedExeSha256) {
    throw 'Release executable differs from the validated executable'
}
if ((Get-FileHash -LiteralPath $ZipPath -Algorithm SHA256).Hash -ne $ExpectedZipSha256) {
    throw 'Release ZIP differs from the validated ZIP'
}
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $ZipPath).Path)
try {
    $entries = @($archive.Entries | Where-Object { $_.FullName -eq 'flx.exe' })
    if ($entries.Count -ne 1) { throw 'Release ZIP must contain exactly one root flx.exe' }
    $stream = $entries[0].Open()
    $sha = [System.Security.Cryptography.SHA256]::Create()
    try {
        $zipExeHash = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '')
        if ($zipExeHash -ne $ExpectedExeSha256) { throw 'ZIP does not contain the validated executable' }
    }
    finally { $sha.Dispose(); $stream.Dispose() }
}
finally { $archive.Dispose() }
Write-Output 'Release executable and ZIP integrity checks passed'
