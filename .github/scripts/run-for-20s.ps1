# Windows counterpart of run-for-20s.sh: start WorkTime, wait, fail on early exit or fatal log output.
param(
  [Parameter(Mandatory)][string]$App,
  [Parameter(Mandatory)][int]$Seconds,
  [Parameter(Mandatory)][string]$Log
)

$ErrorActionPreference = 'Stop'
$errLog = "$Log.err"

$process = Start-Process -FilePath $App -PassThru `
  -RedirectStandardOutput $Log -RedirectStandardError $errLog
Start-Sleep -Seconds $Seconds
$process.Refresh()

if ($process.HasExited) {
  Get-Content $Log, $errLog -ErrorAction SilentlyContinue
  Write-Output "::error::$App exited within $Seconds seconds (exit code $($process.ExitCode))"
  exit 1
}

Stop-Process -Id $process.Id -Force
$output = Get-Content $Log, $errLog -Raw -ErrorAction SilentlyContinue
Write-Output $output

if ($output -match 'FATAL|QCritical') {
  Write-Output "::error::$App logged a fatal error"
  exit 1
}
