# Smoke test of a release zip, on Windows: checks it the way a user would, by unzipping it into an empty directory and
# running it. The same checks as smoke-test.sh.
#
#   .github/scripts/smoke-test.ps1 <aylo-engine-...zip>
#
# Run it on a machine with no toolchain for the answer that counts. Exits 1 if a check fails.
param([Parameter(Mandatory = $true)][string]$Zip)

$ErrorActionPreference = 'Stop'
$here = $PSScriptRoot
$failures = 0

function Ok($message) { Write-Host "ok:   $message" }
function Warn($message) { Write-Host "warn: $message" }
function Fail($message) {
    [Console]::Error.WriteLine("FAIL: $message")
    $script:failures++
}

if (-not (Test-Path -LiteralPath $Zip -PathType Leaf)) {
    [Console]::Error.WriteLine("usage: smoke-test.ps1 <release zip>")
    exit 2
}
$Zip = (Resolve-Path -LiteralPath $Zip).Path
$work = Join-Path ([System.IO.Path]::GetTempPath()) ("aylo-smoke-" + [System.Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
$processes = @()

try {
    # --- Unzip into an empty directory ------------------------------------------------------------------------------
    $unzipped = Join-Path $work 'unzipped'
    Expand-Archive -LiteralPath $Zip -DestinationPath $unzipped
    $entries = @(Get-ChildItem -LiteralPath $unzipped)
    if ($entries.Count -ne 1 -or -not $entries[0].PSIsContainer) {
        [Console]::Error.WriteLine("FAIL: the zip must hold exactly one folder, it holds $($entries.Count) entries")
        exit 1
    }
    $root = $entries[0].FullName
    Ok "unzipped to one folder: $($entries[0].Name)"

    # --- Layout -------------------------------------------------------------------------------------------------------
    $runtime = Join-Path $root 'runtime\aylo-runtime.exe'
    foreach ($file in 'manifest.json', 'README.md', 'VERSION', 'templates\game\project.luau', 'runtime\licenses\THIRD_PARTY_NOTICES.md',
                      'runtime\aylo-runtime.exe', 'tools\aylo-sign.exe') {
        if (Test-Path -LiteralPath (Join-Path $root $file) -PathType Leaf) { Ok "has $file" } else { Fail "missing $file" }
    }
    foreach ($folder in 'runtime\plugins', 'runtime\plugins\backends', 'templates\game\scripts', 'templates\game\assets') {
        if (Test-Path -LiteralPath (Join-Path $root $folder) -PathType Container) { Ok "has $folder\" } else { Fail "missing $folder\" }
    }
    # The game export keeps the notices of the binaries it keeps: each binary it may keep has its own.
    $binaries = @(Get-Item -LiteralPath $runtime -ErrorAction SilentlyContinue) +
                @(Get-ChildItem -LiteralPath (Join-Path $root 'runtime\plugins') -Recurse -File -Filter '*.dll')
    $unnoticed = 0
    foreach ($binary in $binaries) {
        if (-not (Test-Path -LiteralPath (Join-Path $root "runtime\licenses\by-library\$($binary.Name).md") -PathType Leaf)) {
            $unnoticed++
            [Console]::Error.WriteLine("  no notices for $($binary.Name)")
        }
    }
    if ($unnoticed -eq 0) { Ok "every binary has its notices in runtime\licenses\by-library\" } else { Fail "$unnoticed binaries have no notices" }

    # --- Version ------------------------------------------------------------------------------------------------------
    if (Test-Path -LiteralPath $runtime) {
        $reported = (& $runtime --version | Select-Object -First 1)
        $expected = "aylo-engine " + (Get-Content -LiteralPath (Join-Path $root 'VERSION') -TotalCount 1).Trim()
        if ($reported -eq $expected) { Ok "--version says '$reported'" } else { Fail "--version says '$reported', VERSION says '$expected'" }
    }

    # --- Manifest: the hash of every file ----------------------------------------------------------------------------
    $manifest = Get-Content -LiteralPath (Join-Path $root 'manifest.json') -Raw | ConvertFrom-Json
    $bad = 0
    foreach ($entry in $manifest.files) {
        $file = Join-Path $root ($entry.path -replace '/', '\')
        $have = if (Test-Path -LiteralPath $file) { (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLower() } else { '' }
        if ($have -ne $entry.sha256) {
            Fail "manifest: $($entry.path) has hash '$have', the manifest says '$($entry.sha256)'"
            $bad++
        }
    }
    $present = @(Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object { $_.Name -ne 'manifest.json' }).Count
    if ($manifest.files.Count -ne $present) {
        Fail "manifest lists $($manifest.files.Count) files, the zip holds $present"
    } elseif ($bad -eq 0) {
        Ok "manifest: $present files match their hash"
    }

    # --- Nothing outside the runtime folder ---------------------------------------------------------------------------
    $check = Join-Path $here 'check-self-contained.ps1'
    if (Test-Path -LiteralPath $check) {
        & $check (Join-Path $root 'runtime')
        $runtimeStatus = $LASTEXITCODE
        & $check (Join-Path $root 'tools')
        if ($runtimeStatus -eq 0 -and $LASTEXITCODE -eq 0) {
            Ok 'runtime\ and tools\ depend on nothing outside themselves'
        } else {
            Fail 'something resolves outside the zip (listed above)'
        }
    } else {
        Warn "$check does not exist: the dependencies of the runtime are not checked"
    }

    # --- A server and a headless client ---------------------------------------------------------------------------------
    # The output of a Windows program redirected to a file is buffered by its C runtime: a player joining is not seen
    # until the server stops. Both programs are only checked to stay up.
    Warn 'the output of the server is buffered on Windows: the client connecting is not checked, only that both stay up'
    if (Test-Path -LiteralPath $runtime) {
        $port = 20000 + (Get-Random -Maximum 20000)
        $game = Join-Path $root 'templates\game'
        $serverLog = Join-Path $work 'server.log'
        $clientLog = Join-Path $work 'client.log'
        $server = Start-Process -FilePath $runtime -ArgumentList '--server', "`"$game`"", '--port', $port `
            -RedirectStandardOutput $serverLog -RedirectStandardError (Join-Path $work 'server.err') -PassThru -WindowStyle Hidden
        $processes += $server
        Start-Sleep -Seconds 2
        if (-not $server.HasExited) {
            Ok 'the server on templates\game is running'
            $client = Start-Process -FilePath $runtime -ArgumentList "`"$game`"", '--port', $port, '--headless' `
                -RedirectStandardOutput $clientLog -RedirectStandardError (Join-Path $work 'client.err') -PassThru -WindowStyle Hidden
            $processes += $client
            Start-Sleep -Seconds 4
            if ($client.HasExited) {
                Fail "the headless client exited with code $($client.ExitCode): $(Get-Content -Raw -LiteralPath (Join-Path $work 'client.err'))"
            } else {
                Ok 'the headless client is running'
            }
            if ($server.HasExited) { Fail "the server stopped with code $($server.ExitCode)" } else { Ok 'the server is still running' }
        } else {
            Fail "the server exited with code $($server.ExitCode): $(Get-Content -Raw -LiteralPath (Join-Path $work 'server.err'))"
        }
    }

    # --- aylo-sign ----------------------------------------------------------------------------------------------------
    $keys = Join-Path $work 'keys'
    New-Item -ItemType Directory -Path $keys | Out-Null
    Push-Location $keys
    try { & (Join-Path $root 'tools\aylo-sign.exe') keygen smoke | Out-Null } finally { Pop-Location }
    if ((Test-Path (Join-Path $keys 'smoke.pub')) -and (Test-Path (Join-Path $keys 'smoke.key'))) {
        Ok 'aylo-sign keygen makes a key pair'
    } else {
        Fail 'aylo-sign keygen did not make smoke.pub and smoke.key'
    }

    # --- An exported game (tools\aylo-export) ---------------------------------------------------------------------
    # What a game developer ships: the template exported with the zip's runtime, then run from the export alone.
    $exporter = Join-Path $root 'tools\aylo-export.exe'
    if (Test-Path -LiteralPath $exporter) {
        $source = Join-Path $work 'MyGame'
        New-Item -ItemType Directory -Path $source | Out-Null
        Copy-Item -Recurse -LiteralPath (Join-Path $root 'templates\game') -Destination (Join-Path $source 'game')
        $exports = Join-Path $work 'exports'
        & $exporter (Join-Path $source 'game') $exports *> (Join-Path $work 'export.log')
        if ($LASTEXITCODE -eq 0) { Ok 'aylo-export exported templates\game' } else { Fail "aylo-export failed: $(Get-Content -Raw -LiteralPath (Join-Path $work 'export.log'))" }
        $exported = Join-Path $exports 'MyGame'
        foreach ($file in 'MyGame.exe', 'game.pak', 'export.json', 'licenses\THIRD_PARTY_NOTICES.md', 'play.bat') {
            if (Test-Path -LiteralPath (Join-Path $exported $file) -PathType Leaf) { Ok "the export has $file" } else { Fail "the export has no $file" }
        }
        # Only the libraries export.json lists, and each one it lists.
        $json = Get-Content -Raw -LiteralPath (Join-Path $exported 'export.json') -ErrorAction SilentlyContinue
        $listed = ([regex]::Matches([string]$json, '"file": "plugins/')).Count
        $shipped = @(Get-ChildItem -LiteralPath (Join-Path $exported 'plugins') -Recurse -File -Filter '*.dll' -ErrorAction SilentlyContinue)
        $unlisted = @($shipped | Where-Object {
            $relative = 'plugins/' + $_.FullName.Substring((Join-Path $exported 'plugins').Length + 1).Replace('\', '/')
            -not ([string]$json).Contains("""file"": ""$relative""")
        })
        if ($unlisted.Count -eq 0 -and $shipped.Count -eq $listed) {
            Ok "the export ships the $($shipped.Count) libraries export.json lists, and no other"
        } else {
            Fail "the export ships $($shipped.Count) libraries, export.json lists $listed ($($unlisted.Count) not listed)"
        }
        # The program alone, started from another folder: it finds game.pak next to itself. Its output is buffered
        # on Windows, so both programs are only checked to stay up.
        $program = Join-Path $exported 'MyGame.exe'
        if (Test-Path -LiteralPath $program) {
            $exportPort = 20000 + (Get-Random -Maximum 20000)
            $exportServer = Start-Process -FilePath $program -ArgumentList '--server', '--port', $exportPort -WorkingDirectory $work `
                -RedirectStandardOutput (Join-Path $work 'export-server.log') -RedirectStandardError (Join-Path $work 'export-server.err') -PassThru -WindowStyle Hidden
            $processes += $exportServer
            Start-Sleep -Seconds 2
            $exportClient = Start-Process -FilePath $program -ArgumentList 'game.pak', '--port', $exportPort, '--headless' -WorkingDirectory $exported `
                -RedirectStandardOutput (Join-Path $work 'export-client.log') -RedirectStandardError (Join-Path $work 'export-client.err') -PassThru -WindowStyle Hidden
            $processes += $exportClient
            Start-Sleep -Seconds 4
            if ($exportServer.HasExited) {
                Fail "the exported server stopped with code $($exportServer.ExitCode): $(Get-Content -Raw -LiteralPath (Join-Path $work 'export-server.err'))"
            } else {
                Ok 'the exported server, run from its pack, is running'
            }
            if ($exportClient.HasExited) {
                Fail "the exported client exited with code $($exportClient.ExitCode): $(Get-Content -Raw -LiteralPath (Join-Path $work 'export-client.err'))"
            } else {
                Ok 'the exported headless client is running'
            }
        }
    } else {
        Fail 'tools\aylo-export.exe is missing'
    }
} finally {
    foreach ($process in $processes) {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue }
    }
    Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host ''
if ($failures -eq 0) {
    Write-Host "smoke test passed: $(Split-Path -Leaf $Zip)"
    exit 0
}
[Console]::Error.WriteLine("smoke test FAILED: $failures checks")
exit 1
