$suffix = ([string][char]0xAD6C) + ([string][char]0xC870) + ([string][char]0xD654) + '.html'
$target = Get-ChildItem -LiteralPath 'C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth' |
  Where-Object { $_.Name.EndsWith($suffix) } |
  Select-Object -First 1
if (-not $target) {
  throw 'target html not found'
}

$path = $target.FullName
$text = Get-Content -LiteralPath $path -Raw -Encoding UTF8

$text = $text.Replace("      display: inline-flex;'", "      display: inline-flex;")

$text = [regex]::Replace($text, '(?s)    \.factor-card\.is-preview \{.*?    \}', @'
    .factor-card.is-preview {
      border-color: #e2e8f0;
      background: #ffffff;
      box-shadow: none;
    }
'@)

$text = [regex]::Replace($text, '(?s)            <div id="previewTestBox".*?</div>\s*\n\s*<div class="mt-6">', @'
            <div id="previewTestBox" class="mt-4 hidden rounded-2xl border border-amber-200 bg-amber-50/70 px-4 py-3">
              <div class="flex flex-wrap items-center gap-2 text-sm font-semibold text-amber-900">
                <span class="rounded-full bg-amber-100 px-3 py-1 text-xs font-semibold text-amber-800">미리보기</span>
                <span id="previewTestCode" class="font-black"></span>
                <span id="previewTestGroup" class="text-amber-700"></span>
              </div>
              <p id="previewTestSummary" class="mt-2 text-sm leading-7 text-amber-900"></p>
            </div>

            <div class="mt-6">
'@)

$text = $text.Replace(
@'
      const card = document.createElement("div");
      card.className = `factor-card${previewValue ? " is-preview" : ""}`;
'@,
@'
      const card = document.createElement("div");
      card.className = "factor-card";
'@
)

$text = $text.Replace(
@'
          if (isSelected) chipClass += " is-selected button-mapped-highlight";
          if (isPreview) chipClass += " is-preview";
          if (isSelected && isPreview) chipClass += " is-match";
'@,
@'
          if (isSelected) chipClass += " is-selected button-mapped-highlight";
          if (isPreview) chipClass += " is-preview";
          if (isSelected && isPreview) chipClass += " is-match";
'@
)

$text = $text.Replace(
@'
        previewTestCode.textContent = hoveredTestId;
        previewTestGroup.textContent = previewTest.group;
        previewTestSummary.textContent = previewTest.summary;
'@,
@'
        previewTestCode.textContent = `미리보기: ${hoveredTestId}`;
        previewTestGroup.textContent = previewTest.group;
        previewTestSummary.textContent = previewTest.summary;
'@
)

$text = $text.Replace(
@'
        previewTestCode.textContent = "";
        previewTestGroup.textContent = "";
        previewTestSummary.textContent = "";
'@,
@'
        previewTestCode.textContent = "";
        previewTestGroup.textContent = "";
        previewTestSummary.textContent = "";
'@
)

Set-Content -LiteralPath $path -Value $text -Encoding UTF8
