param(
    [string]$Version = "0.2.0"
)

$Root = $PSScriptRoot

$ReleaseDir = Join-Path $Root "release"
$PackageDir = Join-Path $ReleaseDir "FLXEngine-$Version"

Write-Host "Cleaning previous release..."

if (Test-Path $PackageDir) {
    Remove-Item $PackageDir -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null

Write-Host "Copying engine..."

Copy-Item `
    "$Root\bin\Release\flxengine.exe" `
    "$PackageDir\" `
    -Force

Write-Host "Copying schemas..."

Copy-Item `
    "$Root\docs\schemas" `
    "$PackageDir\schemas" `
    -Recurse `
    -Force

Write-Host "Copying TypeScript definitions..."

Copy-Item `
    "$Root\scripts\flx.d.ts" `
    "$PackageDir\" `
    -Force

Write-Host "Copying examples..."

Copy-Item `
    "$Root\examples\pong" `
    "$PackageDir\examples\pong" `
    -Recurse `
    -Force

Copy-Item `
    "$Root\examples\asteroids" `
    "$PackageDir\examples\asteroids" `
    -Recurse `
    -Force

Copy-Item `
    "$Root\examples\arkanoid" `
    "$PackageDir\examples\arkanoid" `
    -Recurse `
    -Force

Write-Host "Copying documentation..."

Copy-Item `
    "$Root\README.md" `
    "$PackageDir\" `
    -Force

if (Test-Path "$Root\LICENSE") {
    Copy-Item "$Root\LICENSE" "$PackageDir\" -Force
}

if (Test-Path "$Root\CHANGELOG.md") {
    Copy-Item "$Root\CHANGELOG.md" "$PackageDir\" -Force
}

$ZipFile = Join-Path $ReleaseDir "FLXEngine-$Version.zip"

if (Test-Path $ZipFile) {
    Remove-Item $ZipFile -Force
}

Write-Host "Creating zip..."

Compress-Archive `
    -Path "$PackageDir\*" `
    -DestinationPath $ZipFile `
    -Force

Write-Host ""
Write-Host "Release created:"
Write-Host $ZipFile