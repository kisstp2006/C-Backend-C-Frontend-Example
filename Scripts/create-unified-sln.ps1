param(
    [string]$SolutionName = "UnifiedHost"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$solutionSlnPath = Join-Path $repoRoot ("{0}.sln" -f $SolutionName)
$solutionSlnxPath = Join-Path $repoRoot ("{0}.slnx" -f $SolutionName)

function Get-VcxProjectGuid {
    param([string]$ProjectPath)

    if (-not (Test-Path $ProjectPath)) {
        throw "Missing C++ project file: $ProjectPath"
    }

    [xml]$projectXml = Get-Content -Path $ProjectPath -Raw
    $projectGuidNode = $projectXml.Project.PropertyGroup.ProjectGuid | Select-Object -First 1

    if (-not $projectGuidNode) {
        throw "ProjectGuid was not found in $ProjectPath"
    }

    $guidText = $projectGuidNode.ToString().Trim()
    if ($guidText.StartsWith("{")) {
        return $guidText.ToUpper()
    }

    return ("{{{0}}}" -f $guidText).ToUpper()
}

function New-GuidText {
    return ("{{{0}}}" -f ([guid]::NewGuid().ToString().ToUpper()))
}

function Get-RelativePath {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $baseFull = [System.IO.Path]::GetFullPath($BasePath)
    if (-not $baseFull.EndsWith("\")) {
        $baseFull += "\"
    }

    $targetFull = [System.IO.Path]::GetFullPath($TargetPath)
    $baseUri = New-Object System.Uri($baseFull)
    $targetUri = New-Object System.Uri($targetFull)
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)
    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace("/", "\\")
}

Push-Location $repoRoot
try {
    Write-Host "[1/4] Generating C++ project files with CMake..."
    cmake -S Backend -B Backend/build | Out-Host

    $backendVcxproj = Join-Path $repoRoot "Backend/build/Backend.vcxproj"
    $engineHostVcxproj = Join-Path $repoRoot "Backend/build/EngineHost.vcxproj"
    $scriptingHostCsproj = Join-Path $repoRoot "ScriptingHost/ScriptingHost.csproj"
    $abstractionsCsproj = Join-Path $repoRoot "Managed/EngineScripting.Abstractions/EngineScripting.Abstractions.csproj"

    if (-not (Test-Path $scriptingHostCsproj)) {
        throw "Missing C# scripting host project file: $scriptingHostCsproj"
    }

    if (-not (Test-Path $abstractionsCsproj)) {
        throw "Missing abstractions project file: $abstractionsCsproj"
    }

    if (Test-Path $solutionSlnPath) {
        Write-Host "[2/4] Removing existing solution: $solutionSlnPath"
        Remove-Item $solutionSlnPath -Force
    }
    if (Test-Path $solutionSlnxPath) {
        Write-Host "[2/4] Removing existing solution: $solutionSlnxPath"
        Remove-Item $solutionSlnxPath -Force
    }

    Write-Host "[3/4] Collecting project metadata..."
    $scriptingHostGuid = New-GuidText
    $abstractionsGuid = New-GuidText
    $nativeFolderGuid = New-GuidText
    $managedFolderGuid = New-GuidText
    $gamesFolderGuid = New-GuidText
    $backendGuid = Get-VcxProjectGuid -ProjectPath $backendVcxproj
    $engineHostGuid = Get-VcxProjectGuid -ProjectPath $engineHostVcxproj

    $gameProjects = @()
    $gameProjectFiles = Get-ChildItem -Path (Join-Path $repoRoot "GameProjects") -Filter *.csproj -Recurse -ErrorAction SilentlyContinue
    foreach ($project in $gameProjectFiles) {
        $relativePath = Get-RelativePath -BasePath $repoRoot -TargetPath $project.FullName
        $gameProjects += [pscustomobject]@{
            Name = [System.IO.Path]::GetFileNameWithoutExtension($project.Name)
            RelativePath = $relativePath
            Guid = New-GuidText
        }
    }

    Write-Host "[4/4] Writing unified solution file..."
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("Microsoft Visual Studio Solution File, Format Version 12.00")
    $lines.Add("# Visual Studio Version 17")
    $lines.Add("VisualStudioVersion = 17.0.31903.59")
    $lines.Add("MinimumVisualStudioVersion = 10.0.40219.1")
    $lines.Add('Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "Backend", "Backend\build\Backend.vcxproj", "' + $backendGuid + '"')
    $lines.Add("EndProject")
    $lines.Add('Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "EngineHost", "Backend\build\EngineHost.vcxproj", "' + $engineHostGuid + '"')
    $lines.Add("EndProject")
    $lines.Add('Project("{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}") = "EngineScripting.Abstractions", "Managed\EngineScripting.Abstractions\EngineScripting.Abstractions.csproj", "' + $abstractionsGuid + '"')
    $lines.Add("EndProject")
    $lines.Add('Project("{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}") = "ScriptingHost", "ScriptingHost\ScriptingHost.csproj", "' + $scriptingHostGuid + '"')
    $lines.Add("EndProject")

    foreach ($game in $gameProjects) {
        $lines.Add('Project("{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}") = "' + $game.Name + '", "' + $game.RelativePath + '", "' + $game.Guid + '"')
        $lines.Add("EndProject")
    }

    $lines.Add('Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "Native", "Native", "' + $nativeFolderGuid + '"')
    $lines.Add("EndProject")
    $lines.Add('Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "Managed", "Managed", "' + $managedFolderGuid + '"')
    $lines.Add("EndProject")
    $lines.Add('Project("{2150E333-8FDC-42A3-9474-1A3956D46DE8}") = "GameProjects", "GameProjects", "' + $gamesFolderGuid + '"')
    $lines.Add("EndProject")
    $lines.Add("Global")
    $lines.Add("    GlobalSection(SolutionConfigurationPlatforms) = preSolution")
    $lines.Add("        Debug|x64 = Debug|x64")
    $lines.Add("        Release|x64 = Release|x64")
    $lines.Add("    EndGlobalSection")
    $lines.Add("    GlobalSection(ProjectConfigurationPlatforms) = postSolution")
    $lines.Add("        $abstractionsGuid.Debug|x64.ActiveCfg = Debug|Any CPU")
    $lines.Add("        $abstractionsGuid.Debug|x64.Build.0 = Debug|Any CPU")
    $lines.Add("        $abstractionsGuid.Release|x64.ActiveCfg = Release|Any CPU")
    $lines.Add("        $abstractionsGuid.Release|x64.Build.0 = Release|Any CPU")
    $lines.Add("        $scriptingHostGuid.Debug|x64.ActiveCfg = Debug|Any CPU")
    $lines.Add("        $scriptingHostGuid.Debug|x64.Build.0 = Debug|Any CPU")
    $lines.Add("        $scriptingHostGuid.Release|x64.ActiveCfg = Release|Any CPU")
    $lines.Add("        $scriptingHostGuid.Release|x64.Build.0 = Release|Any CPU")
    $lines.Add("        $backendGuid.Debug|x64.ActiveCfg = Debug|x64")
    $lines.Add("        $backendGuid.Debug|x64.Build.0 = Debug|x64")
    $lines.Add("        $backendGuid.Release|x64.ActiveCfg = Release|x64")
    $lines.Add("        $backendGuid.Release|x64.Build.0 = Release|x64")
    $lines.Add("        $engineHostGuid.Debug|x64.ActiveCfg = Debug|x64")
    $lines.Add("        $engineHostGuid.Debug|x64.Build.0 = Debug|x64")
    $lines.Add("        $engineHostGuid.Release|x64.ActiveCfg = Release|x64")
    $lines.Add("        $engineHostGuid.Release|x64.Build.0 = Release|x64")

    foreach ($game in $gameProjects) {
        $lines.Add("        $($game.Guid).Debug|x64.ActiveCfg = Debug|Any CPU")
        $lines.Add("        $($game.Guid).Debug|x64.Build.0 = Debug|Any CPU")
        $lines.Add("        $($game.Guid).Release|x64.ActiveCfg = Release|Any CPU")
        $lines.Add("        $($game.Guid).Release|x64.Build.0 = Release|Any CPU")
    }

    $lines.Add("    EndGlobalSection")
    $lines.Add("    GlobalSection(SolutionProperties) = preSolution")
    $lines.Add("        HideSolutionNode = FALSE")
    $lines.Add("    EndGlobalSection")
    $lines.Add("    GlobalSection(NestedProjects) = preSolution")
    $lines.Add("        $backendGuid = $nativeFolderGuid")
    $lines.Add("        $engineHostGuid = $nativeFolderGuid")
    $lines.Add("        $abstractionsGuid = $managedFolderGuid")
    $lines.Add("        $scriptingHostGuid = $managedFolderGuid")

    foreach ($game in $gameProjects) {
        $lines.Add("        $($game.Guid) = $gamesFolderGuid")
    }

    $lines.Add("    EndGlobalSection")
    $lines.Add("EndGlobal")

    $solutionContent = [string]::Join([Environment]::NewLine, $lines)

    Set-Content -Path $solutionSlnPath -Value $solutionContent -Encoding Ascii

    Write-Host "Done. Solution created: $solutionSlnPath"
    Write-Host "Open it in Visual Studio or VS Code and run EngineHost as startup project."
}
finally {
    Pop-Location
}
