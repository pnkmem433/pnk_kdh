$ErrorActionPreference = "Stop"

$root = "C:\WS\vs_kdh\pnk_kdh"
$target = Join-Path $root "report_html"
$pollSeconds = 2
$debounceSeconds = 8
$script:lastDeploy = Get-Date "2000-01-01"
$generatedRegistryPath = Join-Path $target "report_registry.js"

Write-Host "report_html auto deploy watcher started"
Write-Host "watch target: $target"
Write-Host "initial deploy runs once at startup, then html/js saves trigger redeploy"

function Get-WatchedFiles {
    Get-ChildItem -Path $target -Recurse -File |
        Where-Object {
            $_.Extension -in @(".html", ".js") -and
            $_.FullName -ne $generatedRegistryPath
        } |
        Sort-Object FullName
}

function Get-TopicConfig {
    @{
        styling_booth = @{
            folder = "styling_booth"
        }
        smart_plug = @{
            folder = "smart_plug"
        }
        esp_now = @{
            folder = "esp_now"
        }
    }
}

function Get-DateLabelFromFilename {
    param(
        [string]$BaseName
    )

    if ($BaseName -match '^report_(\d{2})(\d{2})(\d{2})$') {
        return ("20{0}-{1}-{2}" -f $matches[1], $matches[2], $matches[3])
    }

    if ($BaseName -match '^(\d{2})(\d{2})(\d{2})[_-]') {
        return ("20{0}-{1}-{2}" -f $matches[1], $matches[2], $matches[3])
    }

    return $BaseName
}

function Get-RecordObjects {
    param(
        [hashtable]$Topic
    )

    $folderPath = Join-Path $target $Topic.folder
    if (-not (Test-Path $folderPath)) {
        return @()
    }

    $files = Get-ChildItem -Path $folderPath -File -Filter "*.html" |
        Where-Object { $_.Name -ne "index.html" } |
        Sort-Object `
            @{ Expression = "LastWriteTime"; Descending = $true }, `
            @{ Expression = "Name"; Descending = $true }

    $records = @()
    foreach ($file in $files) {
        $baseName = [System.IO.Path]::GetFileNameWithoutExtension($file.Name)
        $dateLabel = Get-DateLabelFromFilename -BaseName $baseName

        $records += [ordered]@{
            href = "./$($file.Name)"
            title = $baseName
            dateLabel = $dateLabel
            subtitle = $file.Name
        }
    }

    return $records
}

function Update-ReportRegistry {
    $topics = Get-TopicConfig
    $topicPayload = [ordered]@{}

    foreach ($topicKey in $topics.Keys) {
        $topic = $topics[$topicKey]
        $records = Get-RecordObjects -Topic $topic

        $latestHref = "./index.html"
        $latestTitle = ""
        $latestSubtitle = ""

        if ($records.Count -gt 0) {
            $latestHref = $records[0].href
            $latestTitle = $records[0].title
            $latestSubtitle = $records[0].subtitle
        }

        $topicPayload[$topicKey] = [ordered]@{
            latest = [ordered]@{
                href = $latestHref
                title = $latestTitle
                subtitle = $latestSubtitle
            }
            records = $records
        }
    }

    $payload = [ordered]@{
        generatedAt = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
        topics = $topicPayload
    }

    $json = $payload | ConvertTo-Json -Depth 6
    $content = "window.REPORT_REGISTRY = $json;"
    Set-Content -LiteralPath $generatedRegistryPath -Value $content -Encoding UTF8
}

function Get-FileStateMap {
    $map = @{}
    foreach ($file in Get-WatchedFiles) {
        $map[$file.FullName] = $file.LastWriteTimeUtc.Ticks
    }
    return $map
}

function Get-ChangedFiles {
    param(
        [hashtable]$Previous,
        [hashtable]$Current
    )

    $changes = New-Object System.Collections.Generic.List[string]

    foreach ($path in $Current.Keys) {
        if (-not $Previous.ContainsKey($path)) {
            $changes.Add($path)
            continue
        }

        if ($Previous[$path] -ne $Current[$path]) {
            $changes.Add($path)
        }
    }

    foreach ($path in $Previous.Keys) {
        if (-not $Current.ContainsKey($path)) {
            $changes.Add($path)
        }
    }

    return $changes
}

function Invoke-ReportDeploy {
    param(
        [string]$Reason
    )

    $now = Get-Date
    $script:lastDeploy = $now

    Write-Host ""
    Write-Host ("[{0}] deploy: {1}" -f $now.ToString("yyyy-MM-dd HH:mm:ss"), $Reason)
    Write-Host "running vercel production deploy..."

    Push-Location $root
    try {
        Update-ReportRegistry
        cmd /c npx vercel deploy report_html --prod --yes
    }
    finally {
        Pop-Location
    }
}

$previousState = Get-FileStateMap

Invoke-ReportDeploy -Reason "initial deploy after watcher start"

while ($true) {
    Start-Sleep -Seconds $pollSeconds

    $currentState = Get-FileStateMap
    $changes = Get-ChangedFiles -Previous $previousState -Current $currentState

    if ($changes.Count -gt 0) {
        $elapsed = ((Get-Date) - $script:lastDeploy).TotalSeconds
        if ($elapsed -ge $debounceSeconds) {
            $first = Split-Path -Leaf $changes[0]
            $extra = $changes.Count - 1

            if ($extra -gt 0) {
                Invoke-ReportDeploy -Reason ("file save detected - {0} plus {1} more" -f $first, $extra)
            }
            else {
                Invoke-ReportDeploy -Reason ("file save detected - {0}" -f $first)
            }
        }
    }

    $previousState = $currentState
}
