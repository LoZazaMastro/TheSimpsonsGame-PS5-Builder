# Run this on Windows after preparing the final PS5 payload in Assets.
# Captures the production WPF assembly; never creates substitute UI or a fake DLL.
[CmdletBinding()]
param(
    [string] $WorkDirectory = (Join-Path ([IO.Path]::GetTempPath()) ('SimpsonsBuilderGallery-' + [Guid]::NewGuid().ToString('N'))),
    [string] $DestinationDirectory = (Join-Path $PSScriptRoot 'docs/screenshots'),
    [ValidateSet(96, 120, 144, 192)] [int] $ExpectedMonitorDpi = 96,
    [string] $DotNet = 'dotnet',
    [switch] $VerifyOnly,
    [string] $ExpectedBuilderSha256 = ''
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$names = @('source.png', 'graphics.png', 'frame-generation.png')

function Get-Sha256([string] $Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Get-BuilderSourceInventory {
    $inventory = [ordered]@{}
    foreach ($directory in @('Builder.Wpf', 'Builder.Core', 'Builder.UiTests')) {
        foreach ($file in (Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot $directory) -Recurse -File | Sort-Object FullName)) {
            $relative = $file.FullName.Substring($PSScriptRoot.Length).TrimStart([char[]]'\/').Replace('\', '/')
            if ($relative -match '(^|/)(bin|obj|__pycache__)(/|$)') { continue }
            $inventory[$relative] = Get-Sha256 $file.FullName
        }
    }
    foreach ($relative in @('Directory.Build.props', 'capture-gallery.ps1', 'THIRD-PARTY-NOTICES.txt', 'Assets/PS5-THIRD-PARTY-NOTICES.txt', 'Assets/builder.ico')) {
        $inventory[$relative] = Get-Sha256 (Join-Path $PSScriptRoot $relative)
    }
    return $inventory
}

function Assert-Gallery([string] $Directory) {
    $reportPath = Join-Path $Directory 'gallery-report.json'
    if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf)) {
        throw 'Publication blocked: gallery-report.json is missing. Capture the real Windows WPF application first.'
    }
    $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
    if ($report.status -ne 'MEASURED_NO_FINDINGS' -or $report.windowsGuiExecution -ne 'EXECUTED' -or
        $report.framework -ne 'WPF' -or $report.mode -ne 'gallery' -or
        $report.frameGenerationUnavailableVerified -ne $true -or
        $report.productionAssemblyMatchedPublishedBuild -ne $true -or @($report.captures).Count -ne 3) {
        throw 'Publication blocked: the gallery does not contain a successful real WPF run with the verified production assembly and unavailable frame generation.'
    }
    if ($ExpectedBuilderSha256 -and $report.publishedBuilder.sha256 -ne $ExpectedBuilderSha256.ToLowerInvariant()) {
        throw 'Publication blocked: the gallery belongs to a different published Windows executable.'
    }
    $currentSources = Get-BuilderSourceInventory
    if (@($report.sourceSha256.PSObject.Properties).Count -ne $currentSources.Count) {
        throw 'Publication blocked: the gallery source inventory does not match the current production builder and harness.'
    }
    foreach ($key in $currentSources.Keys) {
        $property = $report.sourceSha256.PSObject.Properties[$key]
        if ($null -eq $property -or $property.Value -ne $currentSources[$key]) {
            throw "Publication blocked: the gallery was captured from different source bytes: $key"
        }
    }
    foreach ($assetName in @('capabilities.json', 'payload.json', 'payload.zip')) {
        $entry = @($report.assets | Where-Object { $_.name -eq $assetName })
        if ($entry.Count -ne 1 -or $entry[0].sha256 -ne (Get-Sha256 (Join-Path $PSScriptRoot ('Assets/' + $assetName)))) {
            throw "Publication blocked: the gallery belongs to different embedded assets: $assetName"
        }
    }
    foreach ($name in $names) {
        $path = Join-Path $Directory $name
        $entry = @($report.captures | Where-Object { $_.path -eq $name })
        if ($entry.Count -ne 1 -or -not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Publication blocked: required real screenshot $name is missing or duplicated in the report."
        }
        if ((Get-Sha256 $path) -ne $entry[0].sha256 -or (Get-Item -LiteralPath $path).Length -ne $entry[0].bytes) {
            throw "Publication blocked: $name does not match its recorded bytes and SHA-256."
        }
        $signature = [IO.File]::ReadAllBytes($path)
        if ($signature.Length -lt 24 -or [BitConverter]::ToString($signature, 0, 8) -ne '89-50-4E-47-0D-0A-1A-0A') {
            throw "Publication blocked: $name is not a PNG capture."
        }
    }
    return $report
}

if ($VerifyOnly) {
    $null = Assert-Gallery ([IO.Path]::GetFullPath($DestinationDirectory))
    Write-Host 'Gallery publication prerequisites passed: all three real WPF PNGs and recorded hashes are present.'
    Write-Host 'Visual review remains required; geometric measurements do not certify readability or native Acrylic.'
    exit 0
}

if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT) {
    throw 'Real WPF gallery capture requires Windows. No screenshots or replacement images were created.'
}
if (Test-Path -LiteralPath $WorkDirectory) {
    throw 'Choose a new work directory. Existing capture logs and output are preserved.'
}
if (Test-Path -LiteralPath $DestinationDirectory) {
    throw 'Choose a new destination directory. Existing gallery images are preserved for review.'
}

$work = [IO.Path]::GetFullPath($WorkDirectory)
$destination = [IO.Path]::GetFullPath($DestinationDirectory)
$null = New-Item -ItemType Directory -Path $work
$env:DOTNET_CLI_TELEMETRY_OPTOUT = '1'
$env:DOTNET_SKIP_FIRST_TIME_EXPERIENCE = '1'
$env:DOTNET_CLI_DO_NOT_USE_MSBUILD_SERVER = '1'
$env:DOTNET_NOLOGO = '1'
$env:MSBUILDDISABLENODEREUSE = '1'

function Invoke-Recorded([string] $Name, [string] $Executable, [string[]] $Arguments) {
    $log = Join-Path $work ($Name + '.log')
    Write-Host "$Name ..."
    & $Executable @Arguments *> $log
    if ($LASTEXITCODE -ne 0) {
        Get-Content -LiteralPath $log | Write-Host
        throw "$Name failed with exit code $LASTEXITCODE. Publication is blocked. Log: $log"
    }
}

$harness = Join-Path $work 'ui-harness'
$published = Join-Path $work 'published-builder'
$captures = Join-Path $work 'captures'
$project = Join-Path $PSScriptRoot 'Builder.Wpf/Builder.Wpf.csproj'
$testProject = Join-Path $PSScriptRoot 'Builder.UiTests/Builder.UiTests.csproj'
$sourceBeforeBuild = Get-BuilderSourceInventory
$assetsBeforeBuild = [ordered]@{}
foreach ($assetName in @('capabilities.json', 'payload.json', 'payload.zip')) {
    $assetsBeforeBuild[$assetName] = Get-Sha256 (Join-Path $PSScriptRoot ('Assets/' + $assetName))
}

# The harness references and builds the real production project. Publishing the
# builder with --no-build then bundles that same compilation without replacing it.
Invoke-Recorded 'publish-harness' $DotNet @(
    'publish', $testProject, '-c', 'Release', '-r', 'win-x64', '--self-contained', 'true',
    '--disable-build-servers', '-m:1', '-p:UseSharedCompilation=false', '-p:BuildInParallel=false', '-o', $harness)
Invoke-Recorded 'publish-production' $DotNet @(
    'publish', $project, '-c', 'Release', '-r', 'win-x64', '--self-contained', 'true',
    '--no-build', '--disable-build-servers', '-m:1', '-p:UseSharedCompilation=false', '-p:BuildInParallel=false', '-o', $published)

$assemblyName = 'TheSimpsonsGame-PS5-Builder.dll'
$harnessAssembly = Join-Path $harness $assemblyName
$productionAssembly = Join-Path $PSScriptRoot ('Builder.Wpf/bin/Release/net10.0-windows/win-x64/' + $assemblyName)
$publishedExe = Join-Path $published 'TheSimpsonsGame-PS5-Builder.exe'
$assemblySha = Get-Sha256 $productionAssembly
if ((Get-Sha256 $harnessAssembly) -ne $assemblySha -or -not (Test-Path -LiteralPath $publishedExe -PathType Leaf)) {
    throw 'The harness does not contain the assembly used for the published builder. Publication is blocked.'
}
Invoke-Recorded 'capture-gallery' (Join-Path $harness 'Builder.UiTests.exe') @('--gallery', $captures, $ExpectedMonitorDpi.ToString())

$layoutPath = Join-Path $captures 'layout-report.json'
$layout = Get-Content -LiteralPath $layoutPath -Raw | ConvertFrom-Json
if ($layout.productionSha256 -ne $assemblySha -or (Get-Sha256 $productionAssembly) -ne $assemblySha -or
    (Get-Sha256 $harnessAssembly) -ne $assemblySha -or @($layout.issues).Count -ne 0) {
    throw 'The captured production assembly changed or the WPF harness reported findings. Inspect the work directory; publication is blocked.'
}
$sourceAfterCapture = Get-BuilderSourceInventory
if ($sourceBeforeBuild.Count -ne $sourceAfterCapture.Count) {
    throw 'The builder source file set changed during compilation or capture. Publication is blocked.'
}
foreach ($key in $sourceBeforeBuild.Keys) {
    if (-not $sourceAfterCapture.Contains($key) -or $sourceBeforeBuild[$key] -ne $sourceAfterCapture[$key]) {
        throw "Builder source changed during compilation or capture: $key. Publication is blocked."
    }
}
foreach ($assetName in $assetsBeforeBuild.Keys) {
    if ($assetsBeforeBuild[$assetName] -ne (Get-Sha256 (Join-Path $PSScriptRoot ('Assets/' + $assetName)))) {
        throw "Embedded asset changed during compilation or capture: $assetName. Publication is blocked."
    }
}

# The public report contains no local account paths, game data or optional DLL.
$report = [ordered]@{
    status = $layout.status
    verifiedUtc = $layout.verifiedUtc
    windowsGuiExecution = $layout.windowsGuiExecution
    framework = $layout.framework
    mode = $layout.mode
    captureMethod = $layout.captureMethod
    frameGenerationUnavailableVerified = $layout.frameGenerationUnavailableVerified
    productionAssembly = $assemblyName
    productionSha256 = $layout.productionSha256
    productionAssemblyMatchedPublishedBuild = $true
    productionIdentityMethod = 'The harness builds the production project. The single-file EXE is published with --no-build from that compilation; the loaded assembly SHA-256 is compared before and after capture.'
    sourcePathBase = 'builder directory containing capture-gallery.ps1'
    sourceSha256 = $sourceBeforeBuild
    publishedBuilder = [ordered]@{ name = 'TheSimpsonsGame-PS5-Builder.exe'; bytes = (Get-Item -LiteralPath $publishedExe).Length; sha256 = (Get-Sha256 $publishedExe) }
    assets = @($assetsBeforeBuild.Keys | ForEach-Object {
        [ordered]@{ name = $_; sha256 = $assetsBeforeBuild[$_] }
    })
    captures = @($layout.captures | ForEach-Object {
        [ordered]@{ name = $_.name; path = $_.path; bytes = $_.bytes; sha256 = $_.sha256;
            dpiX = $_.dpiX; dpiY = $_.dpiY; logicalWidth = $_.logicalWidth; logicalHeight = $_.logicalHeight;
            pixelWidth = $_.pixelWidth; pixelHeight = $_.pixelHeight }
    })
    visualReview = 'REQUIRED: inspect the three PNGs before publication. Geometry alone does not certify readability.'
    limitations = $layout.limitations
}
$report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $captures 'gallery-report.json') -Encoding UTF8
$null = Assert-Gallery $captures

$null = New-Item -ItemType Directory -Path $destination
foreach ($name in $names + @('gallery-report.json')) {
    Copy-Item -LiteralPath (Join-Path $captures $name) -Destination (Join-Path $destination $name)
}
$null = Assert-Gallery $destination
Write-Host "Real WPF gallery saved: $destination"
Write-Host "Production executable and detailed logs: $work"
Write-Host 'Review the PNGs before publication. No game build was started and no Lossless.dll was supplied.'
