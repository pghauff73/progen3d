param(
    [Parameter(Mandatory = $true)]
    [string]$CliPath,
    [string]$OutputRoot = [System.IO.Path]::GetTempPath()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$cli = (Resolve-Path -LiteralPath $CliPath).Path
$repository = Split-Path -Parent $PSScriptRoot
$run = Join-Path $OutputRoot ('progen3d-headless-' + [Guid]::NewGuid().ToString('N'))
$run = (New-Item -ItemType Directory -Path $run -Force).FullName
$utf8 = New-Object System.Text.UTF8Encoding $false

function New-RenderJob {
    param(
        [string]$Name,
        [string]$Grammar,
        [UInt64]$Seed = 42,
        [bool]$Strict = $true,
        [int]$Width = 320,
        [int]$Height = 240
    )
    $manifest = Join-Path $run ($Name + '.json')
    $job = [ordered]@{
        schema_version = 1
        grammar = $Grammar
        seed = $Seed
        width = $Width
        height = $Height
        output_dir = $Name
        strict = $Strict
    }
    [System.IO.File]::WriteAllText($manifest, ($job | ConvertTo-Json), $utf8)
    return $manifest
}

function Invoke-CheckedCli {
    param([string]$Command, [int]$ExpectedExit, [string]$Job)
    $cliArguments = @($Command)
    if ($Job) {
        $cliArguments += $Job
    }
    $lines = @(& $cli @cliArguments)
    $exitCode = $LASTEXITCODE
    if ($lines.Count -ne 1) {
        throw "Expected one JSON line from $Command. Output: $($lines -join [Environment]::NewLine)"
    }
    $report = $lines[0] | ConvertFrom-Json
    if ($exitCode -ne $ExpectedExit -or $report.exit_code -ne $ExpectedExit -or
        $report.ok -ne ($ExpectedExit -eq 0)) {
        throw "Unexpected result from $Command. Exit $exitCode. Report: $($lines[0])"
    }
    return $report
}

function Assert-Preview {
    param($Report, [int]$MinimumPixels = 100)

    if ($Report.pixels_different_from_background -lt $MinimumPixels -or $Report.primitive_count -lt 1) {
        throw "The render had too little visible geometry: $($Report | ConvertTo-Json -Compress)"
    }
    $pixels = [System.IO.File]::ReadAllBytes($Report.image)
    $lf = [char]10
    $expectedHeader = "P6${lf}$($Report.width) $($Report.height)${lf}255$lf"
    $header = [System.Text.Encoding]::ASCII.GetString($pixels, 0, $expectedHeader.Length)
    if ($header -ne $expectedHeader -or
        $pixels.Length -ne $expectedHeader.Length + ($Report.width * $Report.height * 3)) {
        throw "Invalid PPM header or payload length: $($Report.image)"
    }
}

function Assert-NoPreview {
    param([string]$Name)
    if (Test-Path -LiteralPath (Join-Path $run "$Name/preview.ppm") -PathType Leaf) {
        throw "Failing job $Name published an image."
    }
}

$fixture = Join-Path $repository 'examples/SimpleModernBuilding'
$vegetation = Join-Path $fixture 'VegetationBuildingObjects/grammars'
[System.IO.File]::WriteAllText(
    (Join-Path $run 'cube.p3d'),
    "Start -> [ R Width(0.8 2.4) T(0 0.5 0) S(Width 1 1) I(Cube material(navybluemattepaint) 1 0.20) ]",
    $utf8)
[System.IO.File]::WriteAllText(
    (Join-Path $run 'bad.p3d'),
    'Start -> [ I(Cube material(navybluemattepaint) 1 0.20)',
    $utf8)
[System.IO.File]::WriteAllText((Join-Path $run 'empty.p3d'), 'Start -> T(1 0 0)', $utf8)
[System.IO.File]::WriteAllText(
    (Join-Path $run 'collapsed.p3d'),
    'Start -> [ S(0 0 0) I(Cube material(navybluemattepaint) 1 0.20) ]',
    $utf8)

$jobs = [ordered]@{
    cube = (New-RenderJob -Name cube -Grammar 'cube.p3d')
    building = (New-RenderJob -Name building -Grammar (Join-Path $fixture 'Single_Floor_Modern_Building_Windows_Doors.p3d'))
    canopy = (New-RenderJob -Name canopy -Grammar (Join-Path $vegetation 'SMBV01_CourtyardCanopyTree.grammar'))
}
$grassGrammar = Join-Path $vegetation 'SMBV23_FountainGrass.grammar'
$doctor = Invoke-CheckedCli -Command doctor -ExpectedExit 0
$evidence = [ordered]@{ directory = $run; opengl = $doctor.opengl; renders = [ordered]@{} }

# Run from a different directory and exercise paths relative to each manifest.
Push-Location ([System.IO.Path]::GetTempPath())
try {
    foreach ($name in $jobs.Keys) {
        $validated = Invoke-CheckedCli -Command validate -ExpectedExit 0 -Job $jobs[$name]
        Assert-NoPreview -Name $name
        $rendered = Invoke-CheckedCli -Command render -ExpectedExit 0 -Job $jobs[$name]
        Assert-Preview $rendered
        if ($rendered.primitive_count -ne $validated.primitive_count) {
            throw "Validation and rendering disagreed on the primitive count for $name."
        }
        $evidence.renders[$name] = $rendered
    }

    $cubeSha = (Get-FileHash -LiteralPath $evidence.renders.cube.image).Hash
    $repeat = Invoke-CheckedCli -Command render -ExpectedExit 0 -Job $jobs.cube
    if ($repeat.pixel_hash_fnv1a64 -ne $evidence.renders.cube.pixel_hash_fnv1a64 -or
        (Get-FileHash -LiteralPath $repeat.image).Hash -ne $cubeSha) {
        throw 'Repeating the cube job produced different pixels.'
    }
    $differentSeedJob = New-RenderJob -Name cube-second-seed -Grammar 'cube.p3d' -Seed 43
    $differentSeed = Invoke-CheckedCli -Command render -ExpectedExit 0 -Job $differentSeedJob
    Assert-Preview $differentSeed
    if ($differentSeed.pixel_hash_fnv1a64 -eq $repeat.pixel_hash_fnv1a64) {
        throw 'Changing the stochastic cube seed produced identical pixels.'
    }
    $evidence.cube_repeat_sha256 = $cubeSha

    $grassJob = New-RenderJob -Name grass-strict -Grammar $grassGrammar
    $grass = Invoke-CheckedCli -Command render -ExpectedExit 3 -Job $grassJob
    if (($grass.runtime_errors -join ' ') -notmatch "unsupported interface type 'ServiceConnection'") {
        throw 'The strict Fountain Grass failure omitted the original runtime diagnostic.'
    }
    Assert-NoPreview -Name 'grass-strict'
    $evidence.grass_strict = $grass

    $grassPermissiveJob = New-RenderJob -Name grass-permissive -Grammar $grassGrammar -Strict $false
    $grassPermissive = Invoke-CheckedCli -Command render -ExpectedExit 0 -Job $grassPermissiveJob
    Assert-Preview $grassPermissive
    if ($grassPermissive.warnings.Count -lt 1 -or $grassPermissive.runtime_errors.Count -lt 1) {
        throw 'The permissive Fountain Grass render omitted diagnostics.'
    }
    $evidence.renders.grass_permissive = $grassPermissive

    $badJob = New-RenderJob -Name invalid-grammar -Grammar 'bad.p3d' -Strict $false
    $bad = Invoke-CheckedCli -Command render -ExpectedExit 3 -Job $badJob
    if ($bad.diagnostics.Count -lt 1) {
        throw 'The invalid grammar did not produce a structured diagnostic.'
    }
    Assert-NoPreview -Name 'invalid-grammar'

    $emptyJob = New-RenderJob -Name empty-geometry -Grammar 'empty.p3d'
    $empty = Invoke-CheckedCli -Command render -ExpectedExit 4 -Job $emptyJob
    if ($empty.primitive_count -ne 0) {
        throw 'The empty-geometry fixture unexpectedly generated primitives.'
    }
    Assert-NoPreview -Name 'empty-geometry'

    $collapsedJob = New-RenderJob -Name collapsed-geometry -Grammar 'collapsed.p3d'
    $collapsed = Invoke-CheckedCli -Command render -ExpectedExit 4 -Job $collapsedJob
    if ($collapsed.primitive_count -ne 1 -or $collapsed.pixels_different_from_background -ne 0) {
        throw 'The collapsed cube must generate a primitive and render zero model pixels.'
    }
    Assert-NoPreview -Name 'collapsed-geometry'

    $badSize = New-RenderJob -Name invalid-size -Grammar 'cube.p3d' -Width 0
    Invoke-CheckedCli -Command render -ExpectedExit 2 -Job $badSize | Out-Null
    Assert-NoPreview -Name 'invalid-size'
} finally {
    Pop-Location
}

$json = $evidence | ConvertTo-Json -Depth 8
[System.IO.File]::WriteAllText((Join-Path $run 'report.json'), $json, $utf8)
Write-Output $json
