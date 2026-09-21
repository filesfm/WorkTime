<#
.SYNOPSIS
  Regenerates the WorkTime Chocolatey package for a given release version and
  packs it into a .nupkg. Run this on Windows with the Chocolatey CLI (`choco`)
  installed, from a fresh checkout of the release tag being packaged.

.PARAMETER Version
  Release version to package, without the leading "v" (e.g. "0.5.11"). Must
  match an existing GitHub release tag "v<Version>" that already has a
  worktime-v<Version>-x64-setup.exe asset attached.

.EXAMPLE
  ./build-package.ps1 -Version 0.5.11
#>
param(
  [Parameter(Mandatory = $true)]
  [string]$Version
)

$ErrorActionPreference = 'Stop'

$root       = $PSScriptRoot
$assetUrl   = "https://github.com/filesfm/WorkTime/releases/download/v$Version/worktime-v$Version-x64-setup.exe"
$tempFile   = Join-Path $env:TEMP "worktime-v$Version-x64-setup.exe"

Write-Host "Downloading $assetUrl ..."
Invoke-WebRequest -Uri $assetUrl -OutFile $tempFile

$checksum = (Get-FileHash -Path $tempFile -Algorithm SHA256).Hash.ToLower()
Write-Host "SHA256: $checksum"

function Update-Field($Path, $Pattern, $Replacement) {
  (Get-Content -Path $Path -Raw) -replace $Pattern, $Replacement | Set-Content -Path $Path -NoNewline
}

Update-Field (Join-Path $root 'worktime.nuspec') `
  '<version>[^<]+</version>' "<version>$Version</version>"

Update-Field (Join-Path $root 'chocolateyinstall.ps1') `
  "\`$version\s*=\s*'[^']+'" "`$version     = '$Version'"
Update-Field (Join-Path $root 'chocolateyinstall.ps1') `
  "\`$checksum64\s*=\s*'[^']+'" "`$checksum64  = '$checksum'"

Write-Host "Updated nuspec/install script to version $Version. Running choco pack ..."
Push-Location $root
try {
  choco pack worktime.nuspec
}
finally {
  Pop-Location
}

Remove-Item $tempFile -Force
