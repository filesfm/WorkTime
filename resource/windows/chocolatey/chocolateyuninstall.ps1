$ErrorActionPreference = 'Stop'

$packageName = 'worktime'

$key = Get-UninstallRegistryKey -SoftwareName 'WorkTime*' -ErrorAction SilentlyContinue |
  Select-Object -First 1

if (-not $key) {
  Write-Warning "$packageName does not appear to be installed (no matching uninstall registry key)."
  return
}

$uninstallExe = $key.UninstallString -replace '^"|"$', ''

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  file           = $uninstallExe
  silentArgs     = '/S'
  validExitCodes = @(0)
}

Uninstall-ChocolateyPackage @packageArgs
