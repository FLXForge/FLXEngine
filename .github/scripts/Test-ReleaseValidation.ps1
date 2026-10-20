param([Parameter(Mandatory = $true)][string]$ExecutablePath)
$ErrorActionPreference = 'Stop'
$cli = Join-Path $PSScriptRoot 'Test-ReleaseCli.ps1'
$artifact = Join-Path $PSScriptRoot 'Test-ReleaseArtifact.ps1'
$root = Join-Path (Get-Location) ("out/release-guard-tests/" + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $root | Out-Null
$version = (Get-Content VERSION -Raw).Trim()

function Assert-Rejected {
    param([scriptblock]$Operation, [string]$Name)
    $rejected = $false
    try { & $Operation | Out-Null } catch { $rejected = $true }
    if (-not $rejected) { throw "Validation guard accepted invalid input: $Name" }
    Write-Output "[PASS] rejects $Name"
}

function New-FakeCli {
    param([string]$Failure)
    $path = Join-Path $root ("fake-" + $Failure + '.cmd')
    $lines = @('@echo off', 'setlocal EnableDelayedExpansion')
    foreach ($command in @('--version', '--help', 'validate', 'compile')) {
        $lines += ('if "%~1"=="{0}" (' -f $command)
        if ($Failure -eq $command) { $lines += '  exit /b 23' }
        elseif ($command -eq '--version') {
            $value = if ($Failure -eq 'wrong-version') { '0.0.0' } else { $version }
            $lines += "  echo $value"
        }
        elseif ($command -eq '--help') { $lines += '  echo Usage: fixture' }
        elseif ($command -eq 'compile' -and $Failure -ne 'missing-output') {
            $lines += @('  set "compiled=%~3"',
                '  echo fixture>"!compiled!"')
        }
        if ($Failure -ne $command) { $lines += '  exit /b 0' }
        $lines += ')'
    }
    $lines += 'exit /b 24'
    Set-Content -LiteralPath $path -Value $lines -Encoding ascii
    return $path
}

# The successful fixture establishes that negative tests reach the guards.
& $cli -ExecutablePath (New-FakeCli 'success') -OutputDirectory (Join-Path $root 'success')
foreach ($failure in @('--version', '--help', 'validate', 'compile', 'wrong-version', 'missing-output')) {
    $fake = New-FakeCli $failure
    Assert-Rejected { & $cli -ExecutablePath $fake -OutputDirectory (Join-Path $root 'failure') } $failure
}

$exe = (Resolve-Path -LiteralPath $ExecutablePath).Path
$zip = Join-Path $root 'valid.zip'
Compress-Archive -LiteralPath $exe -DestinationPath $zip
$exeHash = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
$zipHash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash
& $artifact -ExecutablePath $exe -ZipPath $zip -ExpectedExeSha256 $exeHash -ExpectedZipSha256 $zipHash
Assert-Rejected { & $artifact -ExecutablePath $exe -ZipPath $zip -ExpectedExeSha256 ('0' * 64) -ExpectedZipSha256 $zipHash } 'modified standalone executable'
Assert-Rejected { & $artifact -ExecutablePath $exe -ZipPath $zip -ExpectedExeSha256 $exeHash -ExpectedZipSha256 ('0' * 64) } 'modified ZIP'
$wrongExe = Join-Path $root 'flx.exe'
Set-Content -LiteralPath $wrongExe -Value 'different executable' -Encoding ascii
$wrongZip = Join-Path $root 'wrong-executable.zip'
Compress-Archive -LiteralPath $wrongExe -DestinationPath $wrongZip
$wrongHash = (Get-FileHash -LiteralPath $wrongZip -Algorithm SHA256).Hash
Assert-Rejected { & $artifact -ExecutablePath $exe -ZipPath $wrongZip -ExpectedExeSha256 $exeHash -ExpectedZipSha256 $wrongHash } 'different executable inside ZIP'
$missingZip = Join-Path $root 'missing-executable.zip'
Compress-Archive -LiteralPath $fake -DestinationPath $missingZip
$missingHash = (Get-FileHash -LiteralPath $missingZip -Algorithm SHA256).Hash
Assert-Rejected { & $artifact -ExecutablePath $exe -ZipPath $missingZip -ExpectedExeSha256 $exeHash -ExpectedZipSha256 $missingHash } 'missing root executable'
Write-Output '[PASS] release validation guards'

# Expected native failures above must not leak into the Actions step exit code.
exit 0
