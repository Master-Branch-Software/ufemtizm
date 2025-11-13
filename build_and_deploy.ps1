# PowerShell build and deployment script for Windows
# Usage: .\build_and_deploy.ps1

param(
    [string]$QtPath = "",
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

$AppName = "UnfuckMyTimeZoneMath"
$BuildDir = "build"
$InstallerDir = "installers"

function Find-QtPath {
    Write-Host "==> Searching for Qt installation..."
    
    # Check if QtPath was provided as parameter
    if ($QtPath -ne "" -and (Test-Path $QtPath)) {
        Write-Host "Using provided Qt path: $QtPath"
        return $QtPath
    }
    
    # Search common Qt installation locations
    $qtLocations = @(
        "$env:USERPROFILE\Qt",
        "C:\Qt"
    )
    
    foreach ($qtBase in $qtLocations) {
        if (Test-Path $qtBase) {
            # Find the latest Qt version
            $versions = Get-ChildItem -Path $qtBase -Directory | 
                Where-Object { $_.Name -match '^\d+\.\d+\.\d+$' } |
                Sort-Object Name -Descending
            
            foreach ($version in $versions) {
                # Prefer MinGW over MSVC
                $mingwDirs = Get-ChildItem -Path $version.FullName -Directory | 
                    Where-Object { $_.Name -match '^mingw' } |
                    Sort-Object Name -Descending
                
                if ($mingwDirs) {
                    return $mingwDirs[0].FullName
                }
                
                # Fallback to MSVC if MinGW not found
                $msvcDirs = Get-ChildItem -Path $version.FullName -Directory | 
                    Where-Object { $_.Name -match '^msvc' } |
                    Sort-Object Name -Descending
                
                if ($msvcDirs) {
                    return $msvcDirs[0].FullName
                }
            }
        }
    }
    
    return $null
}

function Find-MinGWCompiler {
    param([string]$QtInstallPath)
    
    # Extract Qt base directory
    if ($QtInstallPath -match '(.*\\Qt)\\[\d\.]+\\mingw') {
        $qtBase = $Matches[1]
        $toolsPath = Join-Path $qtBase "Tools"
        
        if (Test-Path $toolsPath) {
            $mingwTools = Get-ChildItem -Path $toolsPath -Directory | 
                Where-Object { $_.Name -match '^mingw' } |
                Sort-Object Name -Descending
            
            if ($mingwTools) {
                $mingwPath = $mingwTools[0].FullName
                $gccPath = Join-Path $mingwPath "bin\gcc.exe"
                
                if (Test-Path $gccPath) {
                    return $mingwPath
                }
            }
        }
    }
    
    return $null
}

function Get-ProcessorCount {
    return $env:NUMBER_OF_PROCESSORS
}

# Main script
Write-Host "==> UnfuckMyTimeZoneMath Build Script for Windows" -ForegroundColor Cyan
Write-Host ""

# Find Qt installation
$foundQtPath = Find-QtPath
if (-not $foundQtPath) {
    Write-Host "Error: Could not find Qt installation" -ForegroundColor Red
    Write-Host "Please install Qt or specify the path using: .\build_and_deploy.ps1 -QtPath 'C:\Qt\6.x.x\mingw_xx'" -ForegroundColor Yellow
    exit 1
}

Write-Host "==> Using Qt from: $foundQtPath" -ForegroundColor Green

# Determine if we're using MinGW or MSVC
$usingMinGW = $foundQtPath -match 'mingw'
$generator = if ($usingMinGW) { "MinGW Makefiles" } else { "Visual Studio 17 2022" }

Write-Host "==> Build system: $generator" -ForegroundColor Green

# Find matching MinGW compiler if using MinGW Qt
$mingwCompiler = $null
if ($usingMinGW) {
    $mingwCompiler = Find-MinGWCompiler -QtInstallPath $foundQtPath
    if ($mingwCompiler) {
        Write-Host "==> Using MinGW compiler from: $mingwCompiler" -ForegroundColor Green
        # Add MinGW to PATH for this session
        $env:PATH = "$mingwCompiler\bin;$env:PATH"
    }
    else {
        Write-Host "Warning: Could not find matching MinGW compiler in Qt/Tools" -ForegroundColor Yellow
        Write-Host "         Using system MinGW if available" -ForegroundColor Yellow
    }
}

# Clean build directory
Write-Host "==> Cleaning build directory..." -ForegroundColor Cyan
if (Test-Path $BuildDir) {
    Remove-Item -Path $BuildDir -Recurse -Force
}
New-Item -Path $BuildDir -ItemType Directory | Out-Null

# Configure with CMake
Write-Host "==> Configuring with CMake..." -ForegroundColor Cyan
Push-Location $BuildDir

try {
    $cmakeArgs = @(
        "-DCMAKE_PREFIX_PATH=`"$foundQtPath`"",
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-G", "`"$generator`""
    )
    
    # Add MinGW compiler paths if found
    if ($mingwCompiler) {
        $gccPath = Join-Path $mingwCompiler "bin\gcc.exe"
        $gxxPath = Join-Path $mingwCompiler "bin\g++.exe"
        $cmakeArgs += "-DCMAKE_C_COMPILER=`"$gccPath`""
        $cmakeArgs += "-DCMAKE_CXX_COMPILER=`"$gxxPath`""
    }
    
    $cmakeArgs += ".."
    
    $cmakeCommand = "cmake $($cmakeArgs -join ' ')"
    Write-Host "Running: $cmakeCommand" -ForegroundColor Gray
    Invoke-Expression $cmakeCommand
    
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    
    # Build
    Write-Host "==> Building application..." -ForegroundColor Cyan
    $cpuCount = Get-ProcessorCount
    
    if ($usingMinGW) {
        mingw32-make -j$cpuCount
    }
    else {
        cmake --build . --config $BuildType -j $cpuCount
    }
    
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    
    # Deploy Qt dependencies
    Write-Host "==> Deploying Qt dependencies..." -ForegroundColor Cyan
    $windeployqt = Join-Path $foundQtPath "bin\windeployqt.exe"
    $exePath = if ($usingMinGW) { "$AppName.exe" } else { "$BuildType\$AppName.exe" }
    
    if (Test-Path $windeployqt) {
        & $windeployqt --release $exePath
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Warning: windeployqt failed" -ForegroundColor Yellow
        }
    }
    else {
        Write-Host "Warning: windeployqt not found at $windeployqt" -ForegroundColor Yellow
    }
    
    # Create installer
    Write-Host "==> Creating installer..." -ForegroundColor Cyan
    
    # Check if NSIS is available
    $nsisPath = Get-Command makensis -ErrorAction SilentlyContinue
    if ($nsisPath) {
        cpack -G NSIS -C $BuildType
        
        if ($LASTEXITCODE -ne 0) {
            Write-Host "NSIS packaging failed, creating ZIP instead..." -ForegroundColor Yellow
            cpack -G ZIP -C $BuildType
        }
    }
    else {
        Write-Host "NSIS not found, creating ZIP package..." -ForegroundColor Yellow
        cpack -G ZIP -C $BuildType
    }
    
    # Copy installers
    Write-Host "==> Copying installers..." -ForegroundColor Cyan
    Pop-Location
    
    if (-not (Test-Path $InstallerDir)) {
        New-Item -Path $InstallerDir -ItemType Directory | Out-Null
    }
    
    Get-ChildItem -Path $BuildDir -Filter "*.exe" | Where-Object { $_.Name -ne "$AppName.exe" } | Copy-Item -Destination $InstallerDir -Force
    Get-ChildItem -Path $BuildDir -Filter "*.zip" | Copy-Item -Destination $InstallerDir -Force
    
    # Summary
    Write-Host ""
    Write-Host "==> Build and deployment complete!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Executable: $BuildDir\$exePath" -ForegroundColor Cyan
    Write-Host "Installers: $InstallerDir\" -ForegroundColor Cyan
    
    if (Test-Path $InstallerDir) {
        Get-ChildItem -Path $InstallerDir | ForEach-Object {
            Write-Host "  - $($_.Name)" -ForegroundColor Gray
        }
    }
}
catch {
    Pop-Location
    Write-Host ""
    Write-Host "Error: $_" -ForegroundColor Red
    exit 1
}
