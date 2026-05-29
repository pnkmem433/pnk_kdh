from pathlib import Path


TARGET = Path(
    "C:/WS/vs_kdh/pnk_kdh/report_html/styling_booth/"
    "260528_\uac15\ub3d9\ud604_RFID\ud14c\uc2a4\ud2b8\ucf00\uc774\uc2a4 \uad6c\uc870\ud654.html"
)


text = TARGET.read_text(encoding="utf-8")

text = text.replace(
    """    .test-case-button.is-active {
      border-color: #0284c7;
      background: linear-gradient(135deg, #e0f2fe, #f8fafc);
      box-shadow: 0 0 0 3px rgba(14, 165, 233, 0.14);
      color: #0f172a;
    }

    .test-case-button.is-active .test-case-code {
      color: #0369a1;
    }
""",
    """    .test-case-button {
      position: relative;
    }

    .test-case-button.is-active {
      border-color: #0284c7;
      background: linear-gradient(135deg, #e0f2fe, #f8fafc);
      box-shadow: 0 0 0 3px rgba(14, 165, 233, 0.18), 0 16px 32px rgba(14, 165, 233, 0.12);
      color: #0f172a;
      transform: translateY(-1px);
    }

    .test-case-button.is-active::before {
      content: "";
      position: absolute;
      left: -1px;
      top: 14px;
      bottom: 14px;
      width: 6px;
      border-radius: 9999px;
      background: linear-gradient(180deg, #0ea5e9, #0284c7);
    }

    .test-case-button.is-active .test-case-code {
      color: #0369a1;
    }

    .test-case-button.is-active .test-case-selected-badge {
      opacity: 1;
      transform: translateY(0);
    }
""",
)

text = text.replace(
    """    .condition-chip {
      display: inline-flex;
""",
    """    .test-case-selected-badge {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      border-radius: 9999px;
      border: 1px solid #7dd3fc;
      background: #e0f2fe;
      padding: 0.25rem 0.6rem;
      font-size: 0.7rem;
      font-weight: 800;
      letter-spacing: 0.02em;
      color: #075985;
      opacity: 0;
      transform: translateY(4px);
      transition: 160ms ease;
      pointer-events: none;
    }

    .condition-chip {
      display: inline-flex;
""",
)

text = text.replace(
    """                <button type="button" class="test-case-button rounded-2xl border border-slate-200 bg-white p-4 text-left transition hover:border-sky-300" data-test-id="1-1">
                  <div class="text-xs font-semibold uppercase tracking-[0.16em] text-slate-500">옷 1벌</div>
                  <div class="mt-2 text-lg font-black text-slate-950 test-case-code">1-1</div>
                  <div class="mt-1 text-sm text-slate-600">정면 / 왼쪽 벽걸이 / 벽과 가까움</div>
                </button>""",
    """                <button type="button" class="test-case-button rounded-2xl border border-slate-200 bg-white p-4 text-left transition hover:border-sky-300" data-test-id="1-1">
                  <div class="flex items-start justify-between gap-3">
                    <div class="text-xs font-semibold uppercase tracking-[0.16em] text-slate-500">옷 1벌</div>
                    <span class="test-case-selected-badge">선택됨</span>
                  </div>
                  <div class="mt-2 text-lg font-black text-slate-950 test-case-code">1-1</div>
                  <div class="mt-1 text-sm text-slate-600">정면 / 왼쪽 벽걸이 / 벽과 가까움</div>
                </button>""",
)

text = text.replace(
    """    const buttons = Array.from(document.querySelectorAll(".test-case-button"));
""",
    """    const buttons = Array.from(document.querySelectorAll(".test-case-button"));
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
""",
)

text = text.replace(
    '      selectedTestBadge.textContent = "선택됨";\n',
    '      selectedTestBadge.textContent = `현재 선택: ${testId}`;\n',
)

text = text.replace(
    """      buttons.forEach((button) => {
        button.classList.toggle("is-active", button.dataset.testId === pinnedTestId);
        button.classList.remove("is-preview");
      });
""",
    """      buttons.forEach((button) => {
        const isActive = button.dataset.testId === pinnedTestId;
        button.classList.toggle("is-active", isActive);
        button.classList.remove("is-preview");
        button.setAttribute("aria-pressed", isActive ? "true" : "false");
      });
""",
)

TARGET.write_text(text, encoding="utf-8")
