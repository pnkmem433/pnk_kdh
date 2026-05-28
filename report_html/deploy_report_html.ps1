$ErrorActionPreference = "Stop"

$root = "C:\WS\vs_kdh\pnk_kdh"
$target = Join-Path $root "report_html"
$generatedRegistryPath = Join-Path $target "report_registry.js"

Write-Host "report_html manual deploy started"
Write-Host "target: $target"

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
        Sort-Object LastWriteTime -Descending, Name -Descending

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

Push-Location $root
try {
    Update-ReportRegistry
    Write-Host "report_registry.js updated"
    Write-Host "running single production deploy..."
    cmd /c npx vercel deploy report_html --prod --yes
}
finally {
    Pop-Location
}
