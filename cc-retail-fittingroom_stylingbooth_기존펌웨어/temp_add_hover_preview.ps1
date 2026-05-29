$suffix = ([string][char]0xAD6C) + ([string][char]0xC870) + ([string][char]0xD654) + '.html'
$target = Get-ChildItem -LiteralPath 'C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth' |
  Where-Object { $_.Name.EndsWith($suffix) } |
  Select-Object -First 1
if (-not $target) {
  throw 'target html not found'
}

$path = $target.FullName
$text = Get-Content -LiteralPath $path -Raw -Encoding UTF8

$text = [regex]::Replace($text, '(?s)    \.test-case-button\.is-preview \{.*?    \.button-mapped-highlight \{.*?    \}', @'
    .test-case-button.is-preview {
      border-color: #f59e0b;
      background: linear-gradient(135deg, #fffbeb, #ffffff);
      box-shadow: 0 0 0 3px rgba(245, 158, 11, 0.16);
      color: #0f172a;
    }

    .button-mapped-highlight {
      outline: 3px solid #0ea5e9;
      outline-offset: 2px;
      transform: scale(1.02);
      z-index: 10;
    }
'@)

$text = [regex]::Replace($text, '(?s)    \.factor-card\.is-preview \{.*?    \}', @'
    .factor-card.is-preview {
      border-color: #fcd34d;
      background: #fffbeb;
      box-shadow: 0 0 0 2px rgba(245, 158, 11, 0.12);
    }
'@)

$text = $text.Replace(@'
    .factor-value.is-preview {
      border-color: #c7d2fe;
      background: #e0e7ff;
      color: #4338ca;
    }
'@, @'
    .factor-value.is-preview {
      border-color: #fcd34d;
      background: #fef3c7;
      color: #92400e;
    }

    .factor-value.is-match {
      border-color: #86efac;
      background: #dcfce7;
      color: #166534;
    }

    .condition-chip.is-preview {
      border-color: #fcd34d;
      background: #fef3c7;
      color: #92400e;
    }
'@)

$text = [regex]::Replace($text, '(?s)            <p id="selectedTestSummary" class="mt-4 text-sm leading-7 text-slate-700"></p>\s+<div class="mt-6">', @'
            <p id="selectedTestSummary" class="mt-4 text-sm leading-7 text-slate-700"></p>

            <div id="previewTestBox" class="mt-4 hidden rounded-2xl border border-amber-200 bg-amber-50 p-4">
              <div class="flex items-start justify-between gap-3">
                <div>
                  <div class="text-xs font-semibold uppercase tracking-[0.16em] text-amber-700">Hover Preview</div>
                  <div id="previewTestCode" class="mt-2 text-lg font-black text-amber-950"></div>
                  <p id="previewTestGroup" class="mt-1 text-sm font-semibold text-amber-700"></p>
                </div>
                <span class="rounded-full bg-amber-100 px-3 py-1 text-xs font-semibold text-amber-800">미리보기</span>
              </div>
              <p id="previewTestSummary" class="mt-3 text-sm leading-7 text-amber-900"></p>
            </div>

            <div class="mt-6">
'@)

$scriptPattern = '(?s)    const buttons = Array\.from\(document\.querySelectorAll\("\.test-case-button"\)\);.*?    renderSelectedTest\(pinnedTestId\);'
$newScript = @'
    const buttons = Array.from(document.querySelectorAll(".test-case-button"));
    buttons.forEach((button) => {
      if (!button.querySelector(".test-case-selected-badge")) {
        const firstBlock = button.firstElementChild;
        if (!firstBlock) return;

        const badge = document.createElement("span");
        badge.className = "test-case-selected-badge";
        badge.textContent = "선택됨";

        if (firstBlock.classList && firstBlock.classList.contains("flex")) {
          firstBlock.appendChild(badge);
          return;
        }

        const row = document.createElement("div");
        row.className = "flex items-start justify-between gap-3";
        firstBlock.replaceWith(row);
        row.appendChild(firstBlock);
        row.appendChild(badge);
      }
    });
    const selectedTestCode = document.getElementById("selectedTestCode");
    const selectedTestGroup = document.getElementById("selectedTestGroup");
    const selectedTestBadge = document.getElementById("selectedTestBadge");
    const selectedTestSummary = document.getElementById("selectedTestSummary");
    const previewTestBox = document.getElementById("previewTestBox");
    const previewTestCode = document.getElementById("previewTestCode");
    const previewTestGroup = document.getElementById("previewTestGroup");
    const previewTestSummary = document.getElementById("previewTestSummary");
    const selectedFactorGrid = document.getElementById("selectedFactorGrid");
    const selectedTestConditions = document.getElementById("selectedTestConditions");
    const selectedTestNoteBox = document.getElementById("selectedTestNoteBox");
    const selectedTestNote = document.getElementById("selectedTestNote");

    let pinnedTestId = "1-1";
    let hoveredTestId = null;

    function createFactorCard(label, selectedValue, previewValue) {
      const card = document.createElement("div");
      card.className = `factor-card${previewValue ? " is-preview" : ""}`;

      const title = document.createElement("div");
      title.className = "text-xs font-semibold uppercase tracking-[0.14em] text-slate-500";
      title.textContent = label;

      const chipWrap = document.createElement("div");
      chipWrap.className = "mt-3 flex flex-wrap gap-2";

      const options = factorOptions[label] || [];
      const resolvedSelected = selectedValue || "해당 없음";
      const resolvedPreview = previewValue || "";

      if (!options.length) {
        const fallback = document.createElement("div");
        fallback.className = "factor-value is-selected";
        fallback.textContent = resolvedSelected;
        chipWrap.appendChild(fallback);
      } else {
        options.forEach((option) => {
          const isSelected = option === resolvedSelected;
          const isPreview = resolvedPreview && option === resolvedPreview;
          const chip = document.createElement("div");
          let chipClass = "factor-value";

          if (isSelected) chipClass += " is-selected button-mapped-highlight";
          if (isPreview) chipClass += " is-preview";
          if (isSelected && isPreview) chipClass += " is-match";

          chip.className = chipClass;
          chip.textContent = option;
          chipWrap.appendChild(chip);
        });
      }

      card.appendChild(title);
      card.appendChild(chipWrap);
      return card;
    }

    function renderTestPanel() {
      const selectedTest = testCaseData[pinnedTestId];
      if (!selectedTest) return;

      const previewTest = hoveredTestId && hoveredTestId !== pinnedTestId ? testCaseData[hoveredTestId] : null;

      selectedTestCode.textContent = pinnedTestId;
      selectedTestGroup.textContent = selectedTest.group;
      selectedTestSummary.textContent = selectedTest.summary;
      selectedTestBadge.textContent = `현재 선택: ${pinnedTestId}`;
      selectedTestBadge.className = "rounded-full bg-sky-100 px-3 py-1 text-xs font-semibold text-sky-700";

      if (previewTest) {
        previewTestCode.textContent = hoveredTestId;
        previewTestGroup.textContent = previewTest.group;
        previewTestSummary.textContent = previewTest.summary;
        previewTestBox.classList.remove("hidden");
      } else {
        previewTestCode.textContent = "";
        previewTestGroup.textContent = "";
        previewTestSummary.textContent = "";
        previewTestBox.classList.add("hidden");
      }

      selectedFactorGrid.innerHTML = "";
      factorOrder.forEach((label) => {
        const selectedValue = selectedTest.factors[label] || "해당 없음";
        const previewValue = previewTest ? (previewTest.factors[label] || "해당 없음") : "";
        selectedFactorGrid.appendChild(createFactorCard(label, selectedValue, previewValue));
      });

      selectedTestConditions.innerHTML = "";
      selectedTest.conditions.forEach((condition) => {
        const chip = document.createElement("span");
        chip.className = "condition-chip";
        chip.textContent = condition;
        selectedTestConditions.appendChild(chip);
      });

      if (previewTest) {
        previewTest.conditions.forEach((condition) => {
          const chip = document.createElement("span");
          chip.className = "condition-chip is-preview";
          chip.textContent = `미리보기: ${condition}`;
          selectedTestConditions.appendChild(chip);
        });
      }

      if (selectedTest.note) {
        selectedTestNote.textContent = selectedTest.note;
        selectedTestNoteBox.classList.remove("hidden");
      } else {
        selectedTestNote.textContent = "";
        selectedTestNoteBox.classList.add("hidden");
      }

      buttons.forEach((button) => {
        const isActive = button.dataset.testId === pinnedTestId;
        const isPreview = hoveredTestId && button.dataset.testId === hoveredTestId && hoveredTestId !== pinnedTestId;
        button.classList.toggle("is-active", isActive);
        button.classList.toggle("is-preview", isPreview);
      });
    }

    buttons.forEach((button) => {
      button.addEventListener("click", () => {
        pinnedTestId = button.dataset.testId;
        renderTestPanel();
      });

      button.addEventListener("mouseenter", () => {
        hoveredTestId = button.dataset.testId;
        renderTestPanel();
      });

      button.addEventListener("mouseleave", () => {
        hoveredTestId = null;
        renderTestPanel();
      });
    });

    renderTestPanel();
'@
$text = [regex]::Replace($text, $scriptPattern, $newScript)

Set-Content -LiteralPath $path -Value $text -Encoding UTF8
