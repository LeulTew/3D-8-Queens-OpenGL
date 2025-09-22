#Requires -Version 5.1

<#
.SYNOPSIS
    Build script for 3D 8-Queens OpenGL game on Windows
.DESCRIPTION
    Automates the build process for the 3D 8-Queens puzzle game
    Handles dependency installation and compilation
.PARAMETER Clean
    Clean build artifacts
.PARAMETER Dev
    Install development dependencies
.EXAMPLE
    .\build.ps1
.EXAMPLE
    .\build.ps1 -Clean
.EXAMPLE
    .\build.ps1 -Dev
#>

param(
    [switch]$Clean,
    [switch]$Dev
)

# Configuration
$ProjectName = "3D-8-Queens-OpenGL"
$SourceFile = "main.cpp"
$OutputFile = "main.exe"
$MakeCommand = "g++"

# Colors for output
$Green = "Green"
$Yellow = "Yellow"
$Red = "Red"
$Cyan = "Cyan"
$White = "White"

function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

function Write-Header {
    param([string]$Title)
    Write-Host "`n$("=" * 60)" -ForegroundColor Cyan
    Write-Host "  $Title" -ForegroundColor Cyan
    Write-Host "$("=" * 60)" -ForegroundColor Cyan
}

function Test-Command {
    param([string]$Command)
    try {
        $null = Get-Command $Command -ErrorAction Stop
        return $true
    }
    catch {
        return $false
    }
}

function Install-ChocoPackage {
    param([string]$Package)
    if (-not (Test-Command "choco")) {
        Write-ColorOutput "Chocolatey not found. Installing..." $Yellow
        Set-ExecutionPolicy Bypass -Scope Process -Force
        Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
    }

    if (-not (Test-Path "C:\ProgramData\chocolatey\bin\$Package.exe")) {
        Write-ColorOutput "Installing $Package..." $Yellow
        choco install $Package -y
    }
}

function Check-Dependencies {
    Write-Header "Checking Dependencies"

    $dependencies = @(
        @{Name = "g++"; Command = "g++"; Package = "mingw" },
        @{Name = "make"; Command = "make"; Package = "make" },
        @{Name = "OpenGL"; Command = $null; Package = $null },
        @{Name = "FreeGLUT"; Command = $null; Package = "freeglut" }
    )

    $allGood = $true

    foreach ($dep in $dependencies) {
        Write-Host "Checking $($dep.Name)..." -NoNewline

        if ($dep.Command -and -not (Test-Command $dep.Command)) {
            Write-ColorOutput " MISSING" $Red
            if ($dep.Package) {
                Install-ChocoPackage $dep.Package
            }
            $allGood = $false
        } elseif ($dep.Command) {
            Write-ColorOutput " OK" $Green
        } else {
            # For libraries that don't have direct commands
            Write-ColorOutput " Check manually" $Yellow
        }
    }

    if (-not $allGood) {
        Write-ColorOutput "`nSome dependencies were installed. Please restart PowerShell and run the script again." $Yellow
        exit 1
    }

    Write-ColorOutput "`nAll dependencies satisfied!" $Green
}

function Find-IncludePath {
    param([string]$Library)

    $possiblePaths = @(
        "C:\Program Files\${Library}\include",
        "C:\Program Files (x86)\${Library}\include",
        "C:\mingw64\include",
        "C:\mingw32\include",
        "C:\vcpkg\installed\x64-windows\include"
    )

    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

function Find-LibPath {
    param([string]$Library)

    $possiblePaths = @(
        "C:\Program Files\${Library}\lib",
        "C:\Program Files (x86)\${Library}\lib",
        "C:\mingw64\lib",
        "C:\mingw32\lib",
        "C:\vcpkg\installed\x64-windows\lib"
    )

    foreach ($path in $possiblePaths) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

function Build-Project {
    Write-Header "Building Project"

    # Check if source file exists
    if (-not (Test-Path $SourceFile)) {
        Write-ColorOutput "Error: $SourceFile not found!" $Red
        exit 1
    }

    # Prepare include paths
    $includePaths = @(
        ".\include\stb",
        ".\include\tinyobjloader"
    )

    # Try to find FreeType and OpenAL paths
    $freetypeInclude = Find-IncludePath "FreeType"
    $openalInclude = Find-IncludePath "OpenAL"

    if ($freetypeInclude) { $includePaths += $freetypeInclude }
    if ($openalInclude) { $includePaths += $openalInclude }

    # Prepare library paths
    $libPaths = @()
    $freetypeLib = Find-LibPath "FreeType"
    $openalLib = Find-LibPath "OpenAL"

    if ($freetypeLib) { $libPaths += $freetypeLib }
    if ($openalLib) { $libPaths += $openalLib }

    # Build include flags
    $includeFlags = $includePaths | ForEach-Object { "-I`"$_`"" }
    $libPathFlags = $libPaths | ForEach-Object { "-L`"$_`"" }

    # Libraries to link
    $libraries = @(
        "-lfreetype",
        "-lopenal32",
        "-lglut32",
        "-lglu32",
        "-lopengl32",
        "-lgdi32",
        "-lwinmm"
    )

    # Construct compile command
    $compileCmd = "$MakeCommand $SourceFile -o $OutputFile $includeFlags $libPathFlags $libraries -std=c++17 -Wall"

    Write-ColorOutput "Compiling..." $Cyan
    Write-ColorOutput "Command: $compileCmd" $White

    try {
        Invoke-Expression $compileCmd
        if ($LASTEXITCODE -eq 0) {
            Write-ColorOutput "`nBuild successful! Executable: $OutputFile" $Green
        } else {
            Write-ColorOutput "`nBuild failed with exit code $LASTEXITCODE" $Red
            exit 1
        }
    }
    catch {
        Write-ColorOutput "`nBuild failed: $($_.Exception.Message)" $Red
        exit 1
    }
}

function Run-Project {
    Write-Header "Running Project"

    if (-not (Test-Path $OutputFile)) {
        Write-ColorOutput "Error: $OutputFile not found. Build the project first." $Red
        exit 1
    }

    Write-ColorOutput "Launching $ProjectName..." $Green
    Write-ColorOutput "Controls: Mouse drag to rotate, click to place queens, 'S' for auto-solve" $Cyan

    try {
        & ".\$OutputFile"
    }
    catch {
        Write-ColorOutput "Failed to run executable: $($_.Exception.Message)" $Red
    }
}

function Clean-Build {
    Write-Header "Cleaning Build"

    $filesToClean = @(
        $OutputFile,
        "*.o",
        "*.obj",
        "*.exe",
        "*.pdb",
        "*.ilk"
    )

    foreach ($pattern in $filesToClean) {
        Get-ChildItem -Path . -Filter $pattern -ErrorAction SilentlyContinue | ForEach-Object {
            Write-ColorOutput "Removing $($_.Name)" $Yellow
            Remove-Item $_.FullName -Force
        }
    }

    Write-ColorOutput "Clean complete!" $Green
}

# Main execution
Write-Header "3D 8-Queens OpenGL Game Builder"
Write-ColorOutput "Project: $ProjectName" $Cyan
Write-ColorOutput "Platform: Windows" $Cyan
Write-ColorOutput "Date: $(Get-Date)" $Cyan

if ($Clean) {
    Clean-Build
    exit 0
}

if ($Dev) {
    Check-Dependencies
    Write-ColorOutput "`nDevelopment environment ready!" $Green
    exit 0
}

# Normal build process
Check-Dependencies
Build-Project

Write-ColorOutput "`n" + $("-" * 60) $Cyan
Write-ColorOutput "Build complete! Run with: .\$OutputFile" $Green
Write-ColorOutput "Or use: .\build.ps1 -Run" $Cyan

# Ask if user wants to run
$runChoice = Read-Host "`nWould you like to run the game now? (y/n)"
if ($runChoice -eq "y" -or $runChoice -eq "Y") {
    Run-Project
}