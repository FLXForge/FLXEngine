param(
    [Parameter(Mandatory = $true)][string]$ExecutablePath,
    [string]$OutputDirectory = "build/cli-smoke"
)
$ErrorActionPreference = 'Stop'
$executable = (Resolve-Path -LiteralPath $ExecutablePath).Path
$expectedVersion = (Get-Content -LiteralPath VERSION -Raw).Trim()

function Invoke-Flx {
    param([string[]]$Arguments)
    $output = @(& $executable @Arguments)
    if ($LASTEXITCODE -ne 0) {
        throw "FLX command failed (exit $LASTEXITCODE): $($Arguments -join ' ')"
    }
    return $output
}

$version = (Invoke-Flx -Arguments @('--version')) -join "`n"
if ($version -cne $expectedVersion) { throw "CLI version does not match VERSION: $version" }
$help = (Invoke-Flx -Arguments @('--help')) -join "`n"
if ($help -notmatch 'Usage:') { throw 'CLI help did not contain usage information' }
Invoke-Flx -Arguments @('validate', 'examples/pong.flx') | Write-Output
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$outputFile = Join-Path $OutputDirectory 'pong.flxc'
if (Test-Path -LiteralPath $outputFile) { Remove-Item -LiteralPath $outputFile -Force }
Invoke-Flx -Arguments @('compile', "--output=$outputFile", 'examples/pong.flx') | Write-Output
if (-not (Test-Path -LiteralPath $outputFile) -or (Get-Item -LiteralPath $outputFile).Length -eq 0) {
    throw 'CLI compile did not produce a non-empty compiled project'
}
Write-Output "CLI smoke tests passed for $executable ($expectedVersion)"
