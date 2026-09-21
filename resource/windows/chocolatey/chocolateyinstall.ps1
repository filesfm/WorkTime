$ErrorActionPreference = 'Stop'

$packageName = 'worktime'
$version     = '0.5.10'
$url64       = "https://github.com/filesfm/WorkTime/releases/download/v$version/worktime-v$version-x64-setup.exe"
$checksum64  = '33897df748b004f186ecbf4177f1a21bae690b37a6119f49159fb021aa38d3a3'

$packageArgs = @{
  packageName    = $packageName
  fileType       = 'exe'
  url64bit       = $url64
  checksum64     = $checksum64
  checksumType64 = 'sha256'
  softwareName   = 'WorkTime*'
  silentArgs     = '/S'
  validExitCodes = @(0)
}

Install-ChocolateyPackage @packageArgs
