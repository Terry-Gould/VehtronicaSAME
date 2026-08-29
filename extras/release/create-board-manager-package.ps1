param(
  [string]$Version = "1.0.0",
  [string]$ToolAssetVersion = $Version,
  [string]$Owner = "Terry-Gould",
  [string]$Repository = "VehtronicaSAME",
  [switch]$SkipToolDownloads
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..")
$dist = Join-Path $repoRoot "dist"
$toolArchiveDist = Join-Path $dist "tool-archives"
$indexPath = Join-Path $repoRoot "package_vehtronica_same_index.json"
$releaseBaseUrl = "https://github.com/$Owner/$Repository/releases/download/$Version"
$toolReleaseBaseUrl = "https://github.com/$Owner/$Repository/releases/download/$ToolAssetVersion"

$toolSpecs = @(
  [ordered]@{ SourcePackager = "adafruit"; Name = "arm-none-eabi-gcc"; Version = "9-2019q4" }
  [ordered]@{ SourcePackager = "adafruit"; Name = "bossac"; Version = "1.8.0-48-gb176eee" }
  [ordered]@{ SourcePackager = "adafruit"; Name = "CMSIS"; Version = "5.4.0" }
  [ordered]@{ SourcePackager = "adafruit"; Name = "CMSIS-Atmel"; Version = "1.2.2" }
  [ordered]@{ SourcePackager = "arduino"; Name = "openocd"; Version = "0.11.0-arduino2" }
)

function Reset-Directory {
  param([string]$Path)

  if (Test-Path $Path) {
    Remove-Item -LiteralPath $Path -Recurse -Force
  }
  New-Item -ItemType Directory -Path $Path | Out-Null
}

function New-PlatformArchive {
  param(
    [string]$SourceDirectory,
    [string]$RootDirectoryName,
    [string]$ArchivePath
  )

  $stagingParent = Join-Path $dist "_staging"
  $stagingRoot = Join-Path $stagingParent $RootDirectoryName

  Reset-Directory -Path $stagingParent
  New-Item -ItemType Directory -Path $stagingRoot | Out-Null
  Copy-Item -Path (Join-Path $SourceDirectory "*") -Destination $stagingRoot -Recurse -Force

  if (Test-Path $ArchivePath) {
    Remove-Item -LiteralPath $ArchivePath -Force
  }

  Compress-Archive -Path $stagingRoot -DestinationPath $ArchivePath -CompressionLevel Optimal
  Remove-Item -LiteralPath $stagingParent -Recurse -Force

  $archiveFile = Get-Item $ArchivePath
  $archiveHash = (Get-FileHash -Algorithm SHA256 -Path $ArchivePath).Hash.ToLowerInvariant()

  return [ordered]@{
    archiveFileName = $archiveFile.Name
    checksum = "SHA-256:$archiveHash"
    size = "$($archiveFile.Length)"
    url = "$releaseBaseUrl/$($archiveFile.Name)"
  }
}

function Get-PackageIndex {
  param([string]$Packager)

  if ($Packager -eq "arduino") {
    $packageIndexPath = Join-Path $env:LOCALAPPDATA "Arduino15\package_index.json"
  } elseif ($Packager -eq "adafruit") {
    $packageIndexPath = Join-Path $env:LOCALAPPDATA "Arduino15\package_adafruit_index.json"
  } else {
    throw "Unsupported source packager: $Packager"
  }

  if (!(Test-Path $packageIndexPath)) {
    throw "Package index not found: $packageIndexPath"
  }

  return Get-Content $packageIndexPath -Raw | ConvertFrom-Json
}

function Get-SourceTool {
  param(
    [string]$Packager,
    [string]$Name,
    [string]$Version
  )

  $index = Get-PackageIndex -Packager $Packager
  $package = $index.packages | Where-Object { $_.name -eq $Packager } | Select-Object -First 1
  if ($null -eq $package) {
    throw "Package '$Packager' not found in index."
  }

  $tool = $package.tools | Where-Object { $_.name -eq $Name -and $_.version -eq $Version } | Select-Object -First 1
  if ($null -eq $tool) {
    throw "Tool ${Packager}:$Name@$Version was not found in package index."
  }

  return $tool
}

function Convert-ToolToVehtronica {
  param([object]$SourceTool)

  $systems = @()
  foreach ($system in $SourceTool.systems) {
    $systems += [ordered]@{
      host = $system.host
      url = "$toolReleaseBaseUrl/$($system.archiveFileName)"
      archiveFileName = $system.archiveFileName
      checksum = $system.checksum
      size = "$($system.size)"
    }
  }

  return [ordered]@{
    name = $SourceTool.name
    version = $SourceTool.version
    systems = $systems
  }
}

function Copy-Or-DownloadToolArchive {
  param(
    [string]$Url,
    [string]$ArchiveFileName
  )

  $destination = Join-Path $toolArchiveDist $ArchiveFileName
  if (Test-Path $destination) {
    return
  }

  $stagingPackagePath = Join-Path $env:LOCALAPPDATA "Arduino15\staging\packages\$ArchiveFileName"
  if (Test-Path $stagingPackagePath) {
    Copy-Item -LiteralPath $stagingPackagePath -Destination $destination
    return
  }

  Write-Output "Downloading $ArchiveFileName"
  Invoke-WebRequest -Uri $Url -OutFile $destination
}

Reset-Directory -Path $dist
New-Item -ItemType Directory -Path $toolArchiveDist | Out-Null

$platformArchiveName = "VehtronicaSAME-same-$Version.zip"
$platformArchivePath = Join-Path $dist $platformArchiveName
$platformArchive = New-PlatformArchive `
  -SourceDirectory (Join-Path $repoRoot "same") `
  -RootDirectoryName "VehtronicaSAME-same-$Version" `
  -ArchivePath $platformArchivePath

$toolPackages = @()
$downloadedArchiveNames = New-Object 'System.Collections.Generic.HashSet[string]'

foreach ($toolSpec in $toolSpecs) {
  $sourceTool = Get-SourceTool `
    -Packager $toolSpec.SourcePackager `
    -Name $toolSpec.Name `
    -Version $toolSpec.Version

  $toolPackages += Convert-ToolToVehtronica -SourceTool $sourceTool

  if (!$SkipToolDownloads) {
    foreach ($system in $sourceTool.systems) {
      if ($downloadedArchiveNames.Add($system.archiveFileName)) {
        Copy-Or-DownloadToolArchive -Url $system.url -ArchiveFileName $system.archiveFileName
      }
    }
  }
}

$packageIndex = [ordered]@{
  packages = @(
    [ordered]@{
      name = "vehtronica"
      maintainer = "Terry Gould"
      websiteURL = "https://github.com/$Owner/$Repository"
      email = "terry.gould.public@gmail.com"
      platforms = @(
        [ordered]@{
          name = "Vehtronica SAMD/E Boards"
          architecture = "same"
          version = $Version
          category = "Contributed"
          help = [ordered]@{
            online = "https://github.com/$Owner/$Repository"
          }
          url = $platformArchive.url
          archiveFileName = $platformArchive.archiveFileName
          checksum = $platformArchive.checksum
          size = $platformArchive.size
          boards = @(
            [ordered]@{
              name = "Vehtronica MicroCAN-FD"
            }
          )
          toolsDependencies = @(
            [ordered]@{ packager = "vehtronica"; name = "arm-none-eabi-gcc"; version = "9-2019q4" }
            [ordered]@{ packager = "vehtronica"; name = "bossac"; version = "1.8.0-48-gb176eee" }
            [ordered]@{ packager = "vehtronica"; name = "CMSIS"; version = "5.4.0" }
            [ordered]@{ packager = "vehtronica"; name = "CMSIS-Atmel"; version = "1.2.2" }
            [ordered]@{ packager = "vehtronica"; name = "openocd"; version = "0.11.0-arduino2" }
          )
        }
      )
      tools = $toolPackages
    }
  )
}

$json = $packageIndex | ConvertTo-Json -Depth 100
Set-Content -Path $indexPath -Value $json -Encoding ascii

Write-Output "Created $platformArchivePath"
Write-Output "Created $indexPath"
if (!$SkipToolDownloads) {
  Write-Output "Prepared mirrored tool archives in $toolArchiveDist"
}
if ($ToolAssetVersion -eq $Version) {
  Write-Output "Upload $platformArchiveName and every file in dist/tool-archives/ to GitHub release: $Version"
} else {
  Write-Output "Upload $platformArchiveName to GitHub release: $Version"
  Write-Output "Tool archive URLs point to existing GitHub release: $ToolAssetVersion"
}
Write-Output "Boards Manager URL: https://raw.githubusercontent.com/$Owner/$Repository/main/package_vehtronica_same_index.json"
