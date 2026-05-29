$ErrorActionPreference = 'Stop'

$groups = @(
    @{ key = '1_5'; base = 'ant_base_1_5.csv'; variants = @{ existing = 'ant_base_1_5.csv'; left = 'ant_new_1_5_left.csv'; center = 'ant_new_1_5_center.csv'; right = 'ant_new_1_5_right.csv' } },
    @{ key = '2_1'; base = 'ant_base_2_1.csv'; variants = @{ existing = 'ant_base_2_1.csv'; left = 'ant_new_2_1_left.csv'; center = 'ant_new_2_1_center.csv'; right = 'ant_new_2_1_right.csv' } },
    @{ key = '2_2'; base = 'ant_base_2_2.csv'; variants = @{ existing = 'ant_base_2_2.csv'; left = 'ant_new_2_2_left.csv'; center = 'ant_new_2_2_center.csv'; right = 'ant_new_2_2_right.csv' } },
    @{ key = '2_3'; base = 'ant_base_2_3.csv'; variants = @{ existing = 'ant_base_2_3.csv'; left = 'ant_new_2_3_left.csv'; center = 'ant_new_2_3_center.csv'; right = 'ant_new_2_3_right.csv' } },
    @{ key = '2_4'; base = 'ant_base_2_4.csv'; variants = @{ existing = 'ant_base_2_4.csv'; left = 'ant_new_2_4_left.csv'; center = 'ant_new_2_4_center.csv'; right = 'ant_new_2_4_right.csv' } },
    @{ key = '2_5'; base = 'ant_base_2_5.csv'; variants = @{ existing = 'ant_base_2_5.csv'; left = 'ant_new_2_5_left.csv'; center = 'ant_new_2_5_center.csv'; right = 'ant_new_2_5_right.csv' } },
    @{ key = '2_6'; base = 'ant_base_2_6.csv'; variants = @{ existing = 'ant_base_2_6.csv'; left = 'ant_new_2_6_left.csv'; center = 'ant_new_2_6_center.csv'; right = 'ant_new_2_6_right.csv' } },
    @{ key = '3_2'; base = 'ant_base_3_2.csv'; variants = @{ existing = 'ant_base_3_2.csv'; left = 'ant_new_3_2_left.csv'; center = 'ant_new_3_2_center.csv'; right = 'ant_new_3_2_right.csv' } },
    @{ key = '3_6'; base = 'ant_base_3_6.csv'; variants = @{ existing = 'ant_base_3_6.csv'; left = 'ant_new_3_6_left.csv'; center = 'ant_new_3_6_center.csv'; right = 'ant_new_3_6_right.csv' } }
)

$targetTag = 'AA000010'

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

function Parse-TagMetrics {
    param([string]$Path, [string]$Target)

    $rows = Import-Csv $Path | Where-Object { $_.row_type -eq 'TAG' -and $_.server_tag_id -eq $Target }
    $sessionMap = @{}

    foreach ($row in $rows) {
        $sid = if ([string]::IsNullOrWhiteSpace($row.session_id)) { '1' } else { [string][int]$row.session_id }
        $count = if ([string]::IsNullOrWhiteSpace($row.tag_read_count)) { 0 } else { [int][double]$row.tag_read_count }
        $first = if ([string]::IsNullOrWhiteSpace($row.tag_first_read_time_ms)) { $null } else { [int][double]$row.tag_first_read_time_ms }

        if (-not $sessionMap.ContainsKey($sid)) {
            $sessionMap[$sid] = @{
                count = 0
                first = $null
            }
        }

        if ($count -gt $sessionMap[$sid].count) {
            $sessionMap[$sid].count = $count
        }

        if ($first -ne $null) {
            if ($sessionMap[$sid].first -eq $null -or $first -lt $sessionMap[$sid].first) {
                $sessionMap[$sid].first = $first
            }
        }
    }

    $keys = @($sessionMap.Keys | Sort-Object { [int]$_ })
    $counts = @()
    $firsts = @()

    foreach ($sid in $keys) {
        $counts += @($sessionMap[$sid].count)
        $firsts += @($sessionMap[$sid].first)
    }

    [ordered]@{
        counts = $counts
        firsts = $firsts
        trim_count = Get-TrimmedAverage $counts
        trim_first = Get-TrimmedAverage $firsts
    }
}

function New-LineChartSvg {
    param(
        [string]$Title,
        [string]$XLabel,
        [string]$YLabel,
        [string[]]$Labels,
        [double[]]$Values,
        [string]$OutPath
    )

    $width = 760
    $height = 430
    $marginLeft = 70
    $marginRight = 30
    $marginTop = 45
    $marginBottom = 75
    $plotWidth = $width - $marginLeft - $marginRight
    $plotHeight = $height - $marginTop - $marginBottom

    $maxValue = ($Values | Measure-Object -Maximum).Maximum
    if ($null -eq $maxValue) { $maxValue = 0 }
    $maxVal = [math]::Max(5, [math]::Ceiling(($maxValue + 5) / 10) * 10)
    $ticks = 5

    $points = @()
    for ($i = 0; $i -lt $Labels.Count; $i++) {
        $x = if ($Labels.Count -eq 1) { $marginLeft + ($plotWidth / 2) } else { $marginLeft + ($plotWidth * $i / ($Labels.Count - 1)) }
        $y = $marginTop + $plotHeight - (($Values[$i] / $maxVal) * $plotHeight)
        $points += [pscustomobject]@{
            x = [math]::Round($x, 2)
            y = [math]::Round($y, 2)
            v = $Values[$i]
            label = $Labels[$i]
        }
    }

    $polyline = ($points | ForEach-Object { "$($_.x),$($_.y)" }) -join ' '

    $svg = New-Object System.Text.StringBuilder
    [void]$svg.AppendLine("<svg xmlns='http://www.w3.org/2000/svg' width='$width' height='$height' viewBox='0 0 $width $height'>")
    [void]$svg.AppendLine("<rect width='100%' height='100%' fill='white'/>")
    [void]$svg.AppendLine("<text x='" + ($width / 2) + "' y='24' text-anchor='middle' font-family='Arial' font-size='22'>$Title</text>")

    for ($i = 0; $i -le $ticks; $i++) {
        $tickVal = $maxVal * $i / $ticks
        $y = $marginTop + $plotHeight - (($tickVal / $maxVal) * $plotHeight)
        [void]$svg.AppendLine("<line x1='$marginLeft' y1='$y' x2='" + ($marginLeft + $plotWidth) + "' y2='$y' stroke='#dddddd' stroke-dasharray='3,3'/>")
        [void]$svg.AppendLine("<text x='" + ($marginLeft - 10) + "' y='" + ($y + 5) + "' text-anchor='end' font-family='Arial' font-size='12'>" + [math]::Round($tickVal, 0) + "</text>")
    }

    [void]$svg.AppendLine("<line x1='$marginLeft' y1='$marginTop' x2='$marginLeft' y2='" + ($marginTop + $plotHeight) + "' stroke='black'/>")
    [void]$svg.AppendLine("<line x1='$marginLeft' y1='" + ($marginTop + $plotHeight) + "' x2='" + ($marginLeft + $plotWidth) + "' y2='" + ($marginTop + $plotHeight) + "' stroke='black'/>")
    [void]$svg.AppendLine("<polyline fill='none' stroke='#d62828' stroke-width='3' points='$polyline'/>")

    foreach ($p in $points) {
        [void]$svg.AppendLine("<circle cx='$($p.x)' cy='$($p.y)' r='5' fill='#d62828'/>")
        [void]$svg.AppendLine("<text x='$($p.x)' y='" + ($p.y - 10) + "' text-anchor='middle' font-family='Arial' font-size='12' fill='#d62828'>" + [math]::Round($p.v, 2) + "</text>")
        [void]$svg.AppendLine("<text x='$($p.x)' y='" + ($marginTop + $plotHeight + 22) + "' text-anchor='middle' font-family='Arial' font-size='12'>$($p.label)</text>")
    }

    [void]$svg.AppendLine("<text x='" + ($width / 2) + "' y='" + ($height - 20) + "' text-anchor='middle' font-family='Arial' font-size='14'>$XLabel</text>")
    [void]$svg.AppendLine("<text x='18' y='" + ($height / 2) + "' text-anchor='middle' font-family='Arial' font-size='14' transform='rotate(-90 18 " + ($height / 2) + ")'>$YLabel</text>")
    [void]$svg.AppendLine("</svg>")

    $svg.ToString() | Set-Content -Path $OutPath -Encoding UTF8
}

$inputDir = Join-Path (Get-Location) 'analysis_input'
$outDir = Join-Path (Get-Location) 'analysis_graphs'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$result = [ordered]@{}

foreach ($group in $groups) {
    $groupOut = [ordered]@{}

    foreach ($variant in $group.variants.Keys) {
        $fileName = $group.variants[$variant]
        $groupOut[$variant] = Parse-TagMetrics -Path (Join-Path $inputDir $fileName) -Target $targetTag
    }

    $result[$group.key] = $groupOut

    $labels = @('Existing', 'Left', 'Center', 'Right')
    $existingTrim = if ($null -ne $groupOut['existing'].trim_count) { [double]$groupOut['existing'].trim_count } else { 0.0 }
    $leftTrim = if ($null -ne $groupOut['left'].trim_count) { [double]$groupOut['left'].trim_count } else { 0.0 }
    $centerTrim = if ($null -ne $groupOut['center'].trim_count) { [double]$groupOut['center'].trim_count } else { 0.0 }
    $rightTrim = if ($null -ne $groupOut['right'].trim_count) { [double]$groupOut['right'].trim_count } else { 0.0 }
    $values = @($existingTrim, $leftTrim, $centerTrim, $rightTrim)
    $chartPath = Join-Path $outDir ("antenna_{0}.svg" -f $group.key)

    New-LineChartSvg `
        -Title ("RFID Read Count by Antenna Position ({0})" -f $group.key.Replace('_', '-')) `
        -XLabel 'Antenna Position' `
        -YLabel 'RFID Read Count (10s)' `
        -Labels $labels `
        -Values $values `
        -OutPath $chartPath
}

$jsonPath = Join-Path $outDir 'antenna_metrics.json'
$result | ConvertTo-Json -Depth 6 | Set-Content -Path $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 6
