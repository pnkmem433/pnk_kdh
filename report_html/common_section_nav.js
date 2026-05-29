(function () {
  const buildSectionNav = () => {
    const body = document.body;
    const main = document.querySelector("main");
    if (!body || !main) return false;

    if (!main.id) {
      main.id = "top";
    }

    const theme = body.dataset.sectionTheme || "sky";
    const themeMap = {
      sky: ["hover:border-sky-300", "hover:bg-sky-50", "hover:text-sky-800"],
      emerald: ["hover:border-emerald-300", "hover:bg-emerald-50", "hover:text-emerald-800"],
      violet: ["hover:border-violet-300", "hover:bg-violet-50", "hover:text-violet-800"],
    };
    const hoverClasses = themeMap[theme] || themeMap.sky;

    const slugify = (text) =>
      String(text || "")
        .trim()
        .toLowerCase()
        .replace(/[^\w\u3131-\uD79D]+/g, "-")
        .replace(/^-+|-+$/g, "") || "section";

    const sections = Array.from(main.querySelectorAll("section"));
    const items = sections
      .map((section, index) => {
        const heading = section.querySelector("h1, h2, h3");
        if (!heading) return null;
        if (!section.id) {
          let nextId = slugify(heading.textContent);
          if (document.getElementById(nextId)) {
            nextId = `${nextId}-${index + 1}`;
          }
          section.id = nextId;
        }
        return {
          href: `#${section.id}`,
          label: heading.textContent.trim(),
        };
      })
      .filter(Boolean);

    if (!items.length) return false;

    let navContainer = document.getElementById("sectionNav");
    if (!navContainer) {
      const aside = document.createElement("aside");
      aside.className = "fixed right-4 top-1/2 z-40 hidden -translate-y-1/2 xl:block";
      aside.innerHTML = `
        <div class="w-56 rounded-3xl border border-slate-200 bg-white/95 p-3 shadow-[0_16px_40px_rgba(15,23,42,0.08)] backdrop-blur">
          <div class="mb-2 px-2 text-xs font-bold uppercase tracking-[0.18em] text-slate-500">Sections</div>
          <div id="sectionNav" class="grid gap-2"></div>
        </div>
      `;
      body.appendChild(aside);
      navContainer = aside.querySelector("#sectionNav");
    }

    navContainer.innerHTML = [{ href: "#top", label: "Top" }, ...items]
      .map(
        (item) => `
          <a href="${item.href}" class="rounded-2xl border border-slate-200 bg-slate-50 px-3 py-2 text-xs font-semibold text-slate-700 transition ${hoverClasses.join(" ")}">
            ${item.label}
          </a>
        `,
      )
      .join("");

    if (!document.querySelector("[data-common-top-button]")) {
      const topButton = document.createElement("a");
      topButton.href = "#top";
      topButton.dataset.commonTopButton = "true";
      topButton.className = `fixed bottom-6 right-6 z-40 rounded-full border border-slate-300 bg-white px-4 py-3 text-sm font-semibold text-slate-800 shadow-[0_16px_40px_rgba(15,23,42,0.08)] transition ${hoverClasses[0]} ${hoverClasses[2]}`;
      topButton.textContent = "Top";
      body.appendChild(topButton);
    }

    return true;
  };

  const init = () => {
    if (buildSectionNav()) return;

    const main = document.querySelector("main");
    if (!main) return;

    const observer = new MutationObserver(() => {
      if (buildSectionNav()) {
        observer.disconnect();
      }
    });

    observer.observe(main, { childList: true, subtree: true });
  };

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init, { once: true });
  } else {
    init();
  }
})();
