$ErrorActionPreference = 'Stop'

$files = @(
    'person_3_1.csv',
    'person_3_2.csv',
    'person_3_3.csv',
    'person_3_4.csv',
    'person_3_5.csv'
)

$targets = @(
    'AA000010',
    'AA000019',
    'AA000001',
    'AA000011',
    'AA000015',
    'BB000008',
    '49543030',
    'AA000002'
)

function Get-TrimmedAverage {
    param([object[]]$Values)

    $filtered = @($Values | Where-Object { $_ -ne $null })
    if ($filtered.Count -eq 0) { return $null }
    if ($filtered.Count -le 2) {
        return [math]::Round((($filtered | Measure-Object -Sum).Sum / $filtered.Count), 2)
    }

    $sorted = @($filtered | Sort-Object)
    $inner = @()
    for ($i = 1; $i -lt $sorted.Count - 1; $i++) {
        $inner += $sorted[$i]
    }
    return [math]::Round((($inner | Measure-Object -Sum).Sum / $inner.Count), 2)
}

$base = Join-Path (Get-Location) 'analysis_input'
$summary = [ordered]@{}

foreach ($file in $files) {
    $rows = Import-Csv (Join-Path $base $file) | Where-Object { $_.row_type -eq 'TAG' }
    $sessions = @{}

    foreach ($row in $rows) {
        $sid = if ([string]::IsNullOrWhiteSpace($row.session_id)) { '1' } else { [string][int]$row.session_id }
        if (-not $sessions.ContainsKey($sid)) {
            $sessions[$sid] = @{}
        }

        $tag = $row.server_tag_id
        if ([string]::IsNullOrWhiteSpace($tag)) { continue }

        $count = if ([string]::IsNullOrWhiteSpace($row.tag_read_count)) { 0 } else { [int][double]$row.tag_read_count }
        $first = if ([string]::IsNullOrWhiteSpace($row.tag_first_read_time_ms)) { $null } else { [int][double]$row.tag_first_read_time_ms }

        if (-not $sessions[$sid].ContainsKey($tag)) {
            $sessions[$sid][$tag] = @{
                count = 0
                first = $null
            }
        }

        if ($count -gt $sessions[$sid][$tag].count) {
            $sessions[$sid][$tag].count = $count
        }

        if ($first -ne $null) {
            if ($sessions[$sid][$tag].first -eq $null -or $first -lt $sessions[$sid][$tag].first) {
                $sessions[$sid][$tag].first = $first
            }
        }
    }

    $sessionKeys = @($sessions.Keys | Sort-Object { [int]$_ })
    $sessionOut = @()
    $validCounts = @()
    $uniqueCounts = @()

    foreach ($sid in $sessionKeys) {
        $tags = $sessions[$sid]
        $sessionOut += [pscustomobject]@{
            session_id = [int]$sid
            tags = $tags
        }
        $validCounts += @((@($tags.GetEnumerator() | Where-Object { $_.Value.count -ge 5 }).Count))
        $uniqueCounts += @($tags.Count)
    }

    $trimmed = [ordered]@{}
    foreach ($tag in $targets) {
        $counts = @()
        $firsts = @()
        foreach ($sid in $sessionKeys) {
            if ($sessions[$sid].ContainsKey($tag)) {
                $counts += @($sessions[$sid][$tag].count)
                $firsts += @($sessions[$sid][$tag].first)
            } else {
                $counts += @(0)
                $firsts += @($null)
            }
        }

        $trimmed[$tag] = [ordered]@{
            counts = $counts
            firsts = $firsts
            trim_count = Get-TrimmedAverage $counts
            trim_first = Get-TrimmedAverage $firsts
        }
    }

    $summary[$file] = [ordered]@{
        sessions = $sessionOut
        valid_trim = Get-TrimmedAverage $validCounts
        unique_trim = Get-TrimmedAverage $uniqueCounts
        trimmed = $trimmed
    }
}

$outDir = Join-Path (Get-Location) 'analysis_graphs'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$outPath = Join-Path $outDir 'person_metrics.json'
$summary | ConvertTo-Json -Depth 8 | Set-Content -Path $outPath -Encoding UTF8
$summary | ConvertTo-Json -Depth 8
