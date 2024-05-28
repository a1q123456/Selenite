# Build.ps1

$shadersDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projectDir = Split-Path -Parent $shadersDir | Split-Path -Parent | Split-Path -Parent
$toolsDir = Join-Path $projectDir "ThrityPartyTools"
$dxcDir = Join-Path $toolsDir "DXC"
$dxcZip = Join-Path $dxcDir "dxc.zip"
$dxcUrl = "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2403.2/dxc_2024_03_29.zip"
$outputDir = Join-Path $projectDir "Output" "Shaders"

if (!(Test-Path $dxcDir)) {
    New-Item -ItemType Directory -Force -Path $dxcDir
    Invoke-WebRequest -Uri $dxcUrl -OutFile $dxcZip
    Expand-Archive -Path $dxcZip -DestinationPath $dxcDir -Force
}

New-Item -ItemType Directory -Force -Path $outputDir

$dxcCompiler = Join-Path $dxcDir "bin" "x64" "dxc.exe"

Get-ChildItem -Path $shadersDir -Filter *.hlsl -Recurse | ForEach-Object {
    Write-Host "Compiling $_"
    & $dxcCompiler -T "cs_6_5" -E "main" -Od -Zi -I $projectDir -HV 2021 $_.FullName  -Fd $outputDir\ -Fo (Join-Path $outputDir ($_.BaseName + ".dxil"))
}

