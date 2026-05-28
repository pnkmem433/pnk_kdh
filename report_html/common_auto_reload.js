(function () {
  if (typeof window === "undefined" || typeof fetch !== "function") return;
  if (window.location.protocol === "file:") return;

  const POLL_MS = 3000;
  let lastSignature = null;
  let isChecking = false;

  function buildUrl() {
    const url = new URL(window.location.href);
    url.searchParams.set("_reload_check", String(Date.now()));
    return url.toString();
  }

  function getSignature(response) {
    const etag = response.headers.get("etag") || "";
    const modified = response.headers.get("last-modified") || "";
    const length = response.headers.get("content-length") || "";
    return `${etag}|${modified}|${length}`;
  }

  async function requestSignature() {
    const url = buildUrl();

    try {
      const headResponse = await fetch(url, {
        method: "HEAD",
        cache: "no-store",
      });

      if (headResponse.ok) {
        return getSignature(headResponse);
      }
    } catch (_) {
      // Fall through to GET.
    }

    const getResponse = await fetch(url, {
      method: "GET",
      cache: "no-store",
      headers: {
        "x-report-reload-check": "1",
      },
    });

    if (!getResponse.ok) {
      throw new Error(`reload check failed: ${getResponse.status}`);
    }

    return getSignature(getResponse);
  }

  async function checkForUpdate() {
    if (isChecking) return;
    isChecking = true;

    try {
      const nextSignature = await requestSignature();

      if (!lastSignature) {
        lastSignature = nextSignature;
        return;
      }

      if (nextSignature && nextSignature !== lastSignature) {
        window.location.reload();
      }
    } catch (error) {
      console.error("Auto reload check failed:", error);
    } finally {
      isChecking = false;
    }
  }

  window.addEventListener("focus", () => {
    void checkForUpdate();
  });

  document.addEventListener("visibilitychange", () => {
    if (!document.hidden) {
      void checkForUpdate();
    }
  });

  void checkForUpdate();
  window.setInterval(checkForUpdate, POLL_MS);
})();
