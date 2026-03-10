param(
    [Parameter(Mandatory = $true)]
    [string]$Name,

    [string]$SolutionName = "UnifiedHost"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$templateDir = Join-Path $repoRoot "GameTemplates/DefaultGame"
$targetDir = Join-Path $repoRoot ("GameProjects/{0}" -f $Name)

if (-not (Test-Path $templateDir)) {
    throw "Template folder not found: $templateDir"
}

if (Test-Path $targetDir) {
    throw "Game project already exists: $targetDir"
}

Write-Host "[1/3] Creating game project folder: $targetDir"
New-Item -Path $targetDir -ItemType Directory | Out-Null

Write-Host "[2/3] Copying template files..."
Copy-Item -Path (Join-Path $templateDir "*") -Destination $targetDir -Recurse -Force

$csprojPath = Join-Path $targetDir "DefaultGame.csproj"
$newCsprojPath = Join-Path $targetDir ("{0}.csproj" -f $Name)
Rename-Item -Path $csprojPath -NewName ("{0}.csproj" -f $Name)

$scriptPath = Join-Path $targetDir "GameScript.cs"
$scriptContent = Get-Content -Path $scriptPath -Raw
$scriptContent = $scriptContent.Replace("namespace DefaultGame;", "namespace $Name;")
$scriptContent = $scriptContent.Replace("DefaultGame", $Name)
Set-Content -Path $scriptPath -Value $scriptContent -Encoding Ascii

Write-Host "[3/3] Restoring generated project..."
dotnet restore $newCsprojPath | Out-Host

$solutionScriptPath = Join-Path $scriptDir "create-unified-sln.ps1"
if (Test-Path $solutionScriptPath) {
    Write-Host "[4/4] Refreshing unified solution ($SolutionName)..."
    & powershell -ExecutionPolicy Bypass -File $solutionScriptPath -SolutionName $SolutionName | Out-Host
}

Write-Host "Done. Generated game project: $newCsprojPath"
Write-Host "Build with: dotnet build $newCsprojPath"
