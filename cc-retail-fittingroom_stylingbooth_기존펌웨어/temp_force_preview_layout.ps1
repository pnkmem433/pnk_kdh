$suffix = ([string][char]0xAD6C) + ([string][char]0xC870) + ([string][char]0xD654) + '.html'
$target = Get-ChildItem -LiteralPath 'C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth' |
  Where-Object { $_.Name.EndsWith($suffix) } |
  Select-Object -First 1
if (-not $target) {
  throw 'target html not found'
}

$path = $target.FullName
$text = Get-Content -LiteralPath $path -Raw -Encoding UTF8

$styleOld = @'
    .factor-value.is-preview {
      border-color: #c7d2fe;
      background: #e0e7ff;
      color: #4338ca;
    }
'@
$styleNew = @'
    .factor-value.is-preview {
      border-color: #fcd34d;
      background: #fffbeb;
      color: #b45309;
      box-shadow: 0 0 0 2px rgba(245, 158, 11, 0.16);
    }

    .factor-value.is-match {
      border-color: #86efac;
      background: #dcfce7;
      color: #166534;
      box-shadow: 0 0 0 2px rgba(34, 197, 94, 0.16);
    }
'@
$text = $text.Replace($styleOld, $styleNew)

$markupPattern = '(?s)\s*<div class="mt-3 flex items-start justify-between gap-3">.*?<p id="selectedTestSummary" class="mt-4 text-sm leading-7 text-slate-700"></p>'
$markupNew = @'
            <div class="mt-3 flex items-start justify-between gap-3">
              <div>
                <div id="selectedTestCode" class="text-2xl font-black text-slate-950">1-1</div>
                <p id="selectedTestGroup" class="mt-1 text-sm font-semibold text-sky-700">1. 벽걸이 분산 비교</p>
              </div>
              <div class="flex flex-col items-end gap-2">
                <span id="selectedTestBadge" class="rounded-full bg-sky-100 px-3 py-1 text-xs font-semibold text-sky-700">선택됨</span>
                <div id="previewTestBox" class="hidden rounded-2xl border border-amber-200 bg-amber-50 px-3 py-2">
                  <div class="flex flex-wrap items-center justify-end gap-2 text-xs font-semibold text-amber-900">
                    <span class="rounded-full bg-amber-100 px-3 py-1 text-[11px] font-semibold text-amber-800">미리보기</span>
                    <span id="previewTestCode" class="font-black"></span>
                    <span id="previewTestGroup" class="text-amber-700"></span>
                  </div>
                  <p id="previewTestSummary" class="hidden"></p>
                </div>
              </div>
            </div>

            <p id="selectedTestSummary" class="mt-4 text-sm leading-7 text-slate-700"></p>
'@
$text = [regex]::Replace($text, $markupPattern, $markupNew)

Set-Content -LiteralPath $path -Value $text -Encoding UTF8
