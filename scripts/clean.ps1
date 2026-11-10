[CmdletBinding(SupportsShouldProcess = $true)]
param(
  [string]$ProjectRoot = "",
  [switch]$IncludeLogs = $true,
  [switch]$DryRun
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
  $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
} else {
  $ProjectRoot = (Resolve-Path $ProjectRoot).Path
}

Write-Host "[clean] Project root: $ProjectRoot"
if ($DryRun) {
  Write-Host "[clean] Dry-run mode enabled (no files will be deleted)"
}

function Remove-PathSafe {
  param(
    [string]$PathToDelete
  )

  if (-not (Test-Path -LiteralPath $PathToDelete)) {
    return
  }

  if ($DryRun) {
    Write-Host "[clean] would remove: $PathToDelete"
    return
  }

  if ($PSCmdlet.ShouldProcess($PathToDelete, "Remove-Item")) {
    Remove-Item -LiteralPath $PathToDelete -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "[clean] removed: $PathToDelete"
  }
}

# 1) Remove artifact directories in project root.
$dirs = @(
  (Join-Path $ProjectRoot "build")
)
if ($IncludeLogs) {
  $dirs += (Join-Path $ProjectRoot "logs")
}

foreach ($d in $dirs) {
  Remove-PathSafe -PathToDelete $d
}

# 2) Remove generated files recursively under project root.
$patterns = @(
  "*.bin",
  "*.elf",
  "*.map",
  "*.hex",
  "*.partitions.bin",
  "*.bootloader.bin",
  "*.merged.bin",
  "*.uf2",
  "*.tmp",
  "*.temp",
  "*.bak",
  "*.orig",
  "*.rej",
  "*.d"
)

$files = Get-ChildItem -Path $ProjectRoot -Recurse -File -ErrorAction SilentlyContinue |
  Where-Object {
    $name = $_.Name
    foreach ($pat in $patterns) {
      if ($name -like $pat) { return $true }
    }
    return $false
  }

foreach ($f in $files) {
  Remove-PathSafe -PathToDelete $f.FullName
}

Write-Host "[clean] done"
