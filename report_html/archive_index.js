(function () {
  const registry = window.REPORT_REGISTRY;
  const topicKey = document.body.dataset.topic;

  const topicMeta = {
    styling_booth: {
      archiveTitle: "스타일링 부스 기록 목록",
      archiveDescription: "스타일링 부스 RFID 테스트 보고서와 이후 버전을 이 폴더에서 관리합니다.",
      latestTitle: "최신 스타일링 부스 보고서",
      latestSubtitle: "이 폴더에서 수정 날짜가 가장 최근인 HTML",
      latestBadge: "최신",
      historyTitle: "이전 보고서 기록",
      historyDescription: "이 폴더 안에 있는 모든 HTML 기록을 날짜와 함께 볼 수 있습니다.",
      emptyHistory: "기록 HTML이 아직 없습니다.",
    },
    smart_plug: {
      archiveTitle: "스마트 플러그 기록 목록",
      archiveDescription: "스마트 플러그 관련 최신 보고서와 이후 기록을 이 폴더에서 관리합니다.",
      latestTitle: "최신 스마트 플러그 보고서",
      latestSubtitle: "이 폴더에서 수정 날짜가 가장 최근인 HTML",
      latestBadge: "최신",
      historyTitle: "이전 보고서 기록",
      historyDescription: "이 폴더 안에 있는 모든 HTML 기록을 날짜와 함께 볼 수 있습니다.",
      emptyHistory: "기록 HTML이 아직 없습니다.",
    },
    esp_now: {
      archiveTitle: "ESP-NOW 기록 목록",
      archiveDescription: "ESP-NOW 관련 최신 보고서와 이후 기록을 이 폴더에서 관리합니다.",
      latestTitle: "최신 ESP-NOW 보고서",
      latestSubtitle: "이 폴더에서 수정 날짜가 가장 최근인 HTML",
      latestBadge: "최신",
      historyTitle: "이전 보고서 기록",
      historyDescription: "이 폴더 안에 있는 모든 HTML 기록을 날짜와 함께 볼 수 있습니다.",
      emptyHistory: "기록 HTML이 아직 없습니다.",
    },
  };

  if (!registry || !registry.topics || !registry.topics[topicKey] || !topicMeta[topicKey]) {
    return;
  }

  const topic = registry.topics[topicKey];
  const meta = topicMeta[topicKey];

  const setText = (id, value) => {
    const node = document.getElementById(id);
    if (node) node.textContent = value;
  };

  setText("archive-title", meta.archiveTitle);
  setText("archive-description", meta.archiveDescription);
  setText("history-title", meta.historyTitle);
  setText("history-description", meta.historyDescription);

  const latestLink = document.getElementById("latest-link");
  const latestTitle = document.getElementById("latest-title");
  const latestSubtitle = document.getElementById("latest-subtitle");
  const latestBadge = document.getElementById("latest-badge");

  if (latestLink && latestTitle && latestSubtitle && latestBadge) {
    latestLink.href = topic.latest.href || "./index.html";
    latestTitle.textContent = topic.latest.title || meta.latestTitle;
    latestSubtitle.textContent = topic.latest.subtitle || meta.latestSubtitle;
    latestBadge.textContent = meta.latestBadge;
  }

  const records = document.getElementById("record-list");
  if (!records) return;

  records.innerHTML = "";

  if (!topic.records || !topic.records.length) {
    const empty = document.createElement("div");
    empty.className = "rounded-2xl border border-slate-200 bg-slate-50 px-5 py-4 text-sm text-slate-600";
    empty.textContent = meta.emptyHistory;
    records.appendChild(empty);
    return;
  }

  topic.records.forEach((record) => {
    const link = document.createElement("a");
    link.href = record.href;
    link.className =
      "flex items-center justify-between gap-4 rounded-2xl border border-slate-200 bg-white px-5 py-4 transition hover:bg-slate-50";

    const left = document.createElement("div");

    const title = document.createElement("div");
    title.className = "font-semibold text-slate-950";
    title.textContent = record.title;

    const subtitle = document.createElement("div");
    subtitle.className = "mt-1 text-xs text-slate-600";
    subtitle.textContent = record.subtitle;

    left.appendChild(title);
    left.appendChild(subtitle);

    const badge = document.createElement("span");
    badge.className =
      "shrink-0 rounded-full border border-slate-300 bg-slate-50 px-3 py-1 text-xs font-semibold text-slate-700";
    badge.textContent = record.dateLabel || "-";

    link.appendChild(left);
    link.appendChild(badge);
    records.appendChild(link);
  });
})();
