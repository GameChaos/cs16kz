# Host-side unit tests (no AMXX module deps). Usage:
#   .\scripts\run-tests.ps1
#   .\scripts\run-tests.ps1 -Zig "C:\path\to\zig.exe"

param(
    [string]$Zig = $(if ($env:ZIG) { $env:ZIG } else { "zig" })
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$OutDir = Join-Path $Root "zig-out"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Exe = Join-Path $OutDir "kz_global_api_tests.exe"
$Sources = @(
    "src/kz_path_validate.cpp",
    "src/krp_header_validate.cpp",
    "src/test/test_main.cpp",
    "src/test/path_validate_test.cpp",
    "src/test/krp_validate_test.cpp"
) | ForEach-Object { Join-Path $Root $_ }

Push-Location $Root
try {
    & $Zig c++ -std=c++17 -Isrc/include @Sources -o $Exe
    & $Exe
}
finally {
    Pop-Location
}
