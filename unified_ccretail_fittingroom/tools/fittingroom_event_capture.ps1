$OutputPath = "C:\WS\vs_kdh\fittingroom_event_log.csv"
$ProjectDir = "C:\WS\vs_kdh\pnk_kdh\unified_ccretail_fittingroom"
$PioExe = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\pio.exe"
$CsvInfoLine = "# fittingroom_event_log: csv_row_id=host-side persistent row number, device_event_id=device-local sequence from firmware, event=door_open|door_closed|rfid_session_items, http_status=HTTP status or local negative code, result=OK|FAILED|ALREADY_MATCHED|SKIPPED_EMPTY|LAN_UNAVAILABLE, device_ms=millis() on device, attempt_count=send attempts, detail=extra counters/conditions"
$CsvHeaderLine = "csv_row_id,host_time,device_event_id,event,http_status,result,device_ms,attempt_count,detail"

if (-not (Test-Path $PioExe)) {
  $PioExe = "pio"
}

$outputDir = Split-Path -Parent $OutputPath
if (-not (Test-Path $outputDir)) {
  New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

if (-not (Test-Path $OutputPath)) {
  @($CsvInfoLine, $CsvHeaderLine) | Set-Content -Path $OutputPath
}

$existingLines = Get-Content -Path $OutputPath
if ($existingLines.Count -eq 0) {
  @($CsvInfoLine, $CsvHeaderLine) | Set-Content -Path $OutputPath
  $existingLines = Get-Content -Path $OutputPath
} elseif ($existingLines[0] -ne $CsvInfoLine -or
          ($existingLines.Count -lt 2) -or
          ($existingLines[1] -ne $CsvHeaderLine)) {
  $dataLines = @()
  if ($existingLines.Count -gt 0) {
    $dataLines = $existingLines | Where-Object {
      $_ -and
      ($_ -notlike '# fittingroom_event_log:*') -and
      ($_ -ne "host_time,event_id,event,http_status,result,device_ms,attempt_count,detail") -and
      ($_ -ne $CsvHeaderLine)
    }
  }
  @($CsvInfoLine, $CsvHeaderLine) + $dataLines | Set-Content -Path $OutputPath
  $existingLines = Get-Content -Path $OutputPath
}

$NextRowId = ($existingLines | Where-Object {
  $_ -and
  ($_ -notlike '# fittingroom_event_log:*') -and
  ($_ -ne $CsvHeaderLine)
}).Count + 1

Write-Host "Capturing fitting room API events to $OutputPath"
Write-Host "Project: $ProjectDir"

& $PioExe device monitor --project-dir $ProjectDir 2>&1 | ForEach-Object {
  $line = $_.ToString()
  Write-Host $line

  if ($line -match '###AI_CSV###,([^,]+),([^,]+),(-?\d+),([^,]+),(\d+),(\d+),(.*)$') {
    $hostTime = Get-Date -Format "yyyy-MM-ddTHH:mm:ss.fffK"
    $deviceEventId = $matches[1]
    $event = $matches[2]
    $httpStatus = $matches[3]
    $result = $matches[4]
    $deviceMs = $matches[5]
    $attemptCount = $matches[6]
    $detail = $matches[7].Replace('"', "'")

    Add-Content -Path $OutputPath -Value (
      '"' + $NextRowId + '",' +
      '"' + $hostTime + '",' +
      '"' + $deviceEventId + '",' +
      '"' + $event + '",' +
      '"' + $httpStatus + '",' +
      '"' + $result + '",' +
      '"' + $deviceMs + '",' +
      '"' + $attemptCount + '",' +
      '"' + $detail + '"'
    )
    $NextRowId++
  }
}

