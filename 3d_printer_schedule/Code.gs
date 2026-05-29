// ============================================================
// 🖨 3D 프린터 예약 관리 시스템 - 통합 백엔드 (Code.gs)
// ============================================================

var STL_FOLDER_ID = "1nmJhJqeatl08GMa60WB46YqJbjJcdXk6";

var CFG = {
  SCHEDULE_SHEET: "", INPUT_SHEET: "",
  START_HOUR: 9, END_HOUR: 32, ROW_HEIGHT: 38, TIME_COL_W: 90, PRINTER_COL_W: 215,
  COL_USER: 1, COL_PRINTER: 2, COL_DATE: 3, COL_RES_START: 4, COL_RES_DUR: 5,
  COL_ITEM: 6, COL_REASON: 7, COL_ACT_START: 8, COL_ACT_DUR: 9, COL_FILE: 10,
  COL_STATUS: 11, COL_VISIBLE: 12, COL_FILAMENT: 13, COL_MEMO: 14, COL_LABEL: 15,

  PRINTER_EMOJI: {
    "Cubicon 3DP-310F":        "🟦",
    "Cubicon Style-NEO A31C":  "🟥",
    "Cubicon Dual Plus S30i":  "🟩"
  },
  PRINTER_LABEL_STYLE: {
    "Cubicon 3DP-310F":        { bg: "#D4E6F7", font: "#1A4A7A" },
    "Cubicon Style-NEO A31C":  { bg: "#FFD5D5", font: "#7A0000" },
    "Cubicon Dual Plus S30i":  { bg: "#D5F0D5", font: "#1A5C1A" }
  },
  PRINTER_NAMES: ["Cubicon 3DP-310F", "Cubicon Style-NEO A31C", "Cubicon Dual Plus S30i"],
  USER_COLORS: {
    "강동현": { bg: "#D4E6F7", font: "#1A4A7A", border: "#85B7EB" },
    "정재진": { bg: "#D5F0D5", font: "#1A5C1A", border: "#97C459" },
    "정세빈": { bg: "#FDE8CC", font: "#7A3E00", border: "#EF9F27" },
    "성보경": { bg: "#F5D5F0", font: "#6A1A65", border: "#D093C8" },
    "김건우": { bg: "#D5EEF5", font: "#0D4D60", border: "#5DCAA5" },
    "배윤선": { bg: "#FFF0CC", font: "#7A5500", border: "#FAC775" }
  },
  EMPTY_BG: "#FFFFFF", CONFLICT_BG: "#FFB3B3"
};

// ── 시트 이름 저장/로드 ────────────────────────────────────
function saveSheetNames(schedName, inputName) {
  schedName = schedName || ""; inputName = inputName || "";
  if (!schedName || !inputName) return;
  var p = PropertiesService.getScriptProperties();
  p.setProperty("SCHEDULE_SHEET", schedName);
  p.setProperty("INPUT_SHEET",    inputName);
  CFG.SCHEDULE_SHEET = schedName;
  CFG.INPUT_SHEET    = inputName;
}
function loadSheetNames() {
  var p = PropertiesService.getScriptProperties();
  var s = p.getProperty("SCHEDULE_SHEET");
  var i = p.getProperty("INPUT_SHEET");
  if (s) CFG.SCHEDULE_SHEET = s;
  if (i) CFG.INPUT_SHEET    = i;
}

function pad(n) { return String(n).padStart(2, "0"); }

// ══════════════════════════════════════════════════════════
// O열 자동 레이블 생성
// ══════════════════════════════════════════════════════════
function updateLabelCell(sheet, rowNum, preloadedRow) {
  var row = preloadedRow || sheet.getRange(rowNum, 1, 1, CFG.COL_MEMO).getValues()[0];

  var userName = String(row[CFG.COL_USER      - 1] || "").trim();
  var printer  = String(row[CFG.COL_PRINTER   - 1] || "").trim();
  var resStart =        row[CFG.COL_RES_START - 1];
  var resDur   =        row[CFG.COL_RES_DUR   - 1];
  var actStart =        row[CFG.COL_ACT_START - 1];
  var actDur   =        row[CFG.COL_ACT_DUR   - 1];
  var status   = String(row[CFG.COL_STATUS    - 1] || "").trim();
  var item     = String(row[CFG.COL_ITEM      - 1] || "").trim();
  if (item === "undefined") item = "";

  var labelCell   = sheet.getRange(rowNum, CFG.COL_LABEL);
  var hasReserved = !!(resStart && resDur);
  var hasActual   = !!(actStart && actDur);

  // 필수 정보 없으면 빈칸
  if ((!hasReserved && !hasActual) || !userName || !printer) {
    labelCell.setValue("").setBackground("#FFFFFF").setFontColor("#333333").setFontWeight("normal");
    return;
  }

  var style     = CFG.PRINTER_LABEL_STYLE[printer] || { bg: "#F0F0F0", font: "#333333" };
  var emoji     = CFG.PRINTER_EMOJI[printer] || "🟦";
  var suffixRes = (item ? " / " + item : "") + " / " + printer;
  var suffixAct = suffixRes + (status ? " [" + status + "]" : "");

  if (hasActual && hasReserved) {
    // 예약 + 실사용 둘 다
    labelCell.setValue("⬜ [예약] " + userName + suffixRes +
                       "\n" + emoji + " [실사용] " + userName + suffixAct)
      .setBackground("#FFFFFF").setFontColor(style.font).setFontWeight("bold").setWrap(true);
  } else if (hasActual) {
    // 실사용만
    labelCell.setValue(emoji + " [3D프린팅] " + userName + suffixAct)
      .setBackground("#FFFFFF").setFontColor(style.font).setFontWeight("bold");
  } else {
    // 예약만
    labelCell.setValue("⬜ [3D프린팅] " + userName + suffixRes)
      .setBackground("#FFFFFF").setFontColor("#555555").setFontWeight("normal");
  }
}

function refreshAllLabels() {
  loadSheetNames();
  var ss         = SpreadsheetApp.getActiveSpreadsheet();
  var inputSheet = ss.getSheetByName(CFG.INPUT_SHEET) || ss.getSheets()[1];
  var lastRow    = inputSheet.getLastRow();
  if (lastRow < 2) return;
  for (var r = 2; r <= lastRow; r++) updateLabelCell(inputSheet, r);
}

// ══════════════════════════════════════════════════════════
// onEdit — O열 자동갱신 + 변경 타임스탬프 저장
// ══════════════════════════════════════════════════════════
function onEdit(e) {
  try {
    if (!e || !e.range) return;
    loadSheetNames();

    var ss    = SpreadsheetApp.getActiveSpreadsheet();
    var sheet = e.range.getSheet();
    var inputSheetName = CFG.INPUT_SHEET ||
      (ss.getSheets().length >= 2 ? ss.getSheets()[1].getName() : "");
    if (sheet.getName() !== inputSheetName) return;

    var col = e.range.getColumn();
    var row = e.range.getRow();
    if (row < 2) return;

    // O열(15) 이상은 자동생성열 — 무한루프 방지
    if (col >= CFG.COL_LABEL) return;

    // 행 전체 읽기 + 방금 수정된 값 즉시 반영
    var allVals = sheet.getRange(row, 1, 1, CFG.COL_MEMO).getValues()[0];
    allVals[col - 1] = (e.value === undefined || e.value === null) ? "" : e.value;

    // O열 레이블 갱신
    updateLabelCell(sheet, row, allVals);

    // HTML 뷰어 폴링용 변경 타임스탬프 저장
    PropertiesService.getScriptProperties().setProperty("LAST_EDIT_TS", String(Date.now()));

  } catch (err) {
    console.error("onEdit error:", err.toString());
  }
}

// ══════════════════════════════════════════════════════════
// 웹앱 — HTML 뷰어에 시트 데이터 제공
// ══════════════════════════════════════════════════════════
function fmtCellTime(val) {
  if (!val || val === "") return null;
  if (val instanceof Date) return pad(val.getHours()) + ":" + pad(val.getMinutes());
  var s = String(val).trim();
  return s.match(/^\d{1,2}:\d{2}$/) ? s : (s || null);
}

function fmtCellDur(val) {
  if (!val || val === "") return null;
  if (val instanceof Date) return pad(val.getHours()) + ":" + pad(val.getMinutes());
  var s = String(val).trim();
  if (s.match(/^\d{1,2}:\d{2}$/)) return s;
  var n = parseFloat(s);
  if (!isNaN(n) && n > 0) {
    var h = Math.floor(n), mn = Math.min(Math.round((n-h)*100), 59);
    return h + ":" + pad(mn);
  }
  return s || null;
}

function buildWebResponse_(payload, callback) {
  callback = String(callback || "").trim();
  if (callback) {
    var safeCallback = callback.replace(/[^0-9A-Za-z_$.]/g, "");
    return ContentService
      .createTextOutput(safeCallback + "(" + JSON.stringify(payload) + ");")
      .setMimeType(ContentService.MimeType.JAVASCRIPT);
  }
  return ContentService
    .createTextOutput(JSON.stringify(payload))
    .setMimeType(ContentService.MimeType.JSON);
}

function doGet(e) {
  var mode     = (e && e.parameter && e.parameter.mode)     ? e.parameter.mode     : "data";
  var callback = (e && e.parameter && e.parameter.callback) ? e.parameter.callback : "";

  // mode=ts → 타임스탬프만 반환 (빠른 폴링용)
  if (mode === "ts") {
    var ts = PropertiesService.getScriptProperties().getProperty("LAST_EDIT_TS") || "0";
    return buildWebResponse_({ ts: ts }, callback);
  }

  // mode=data → 시트 전체 데이터
  if (mode === "data") {
    return buildWebResponse_(getSheetDataForViewer(), callback);
  }

  // mode=stl → 구글 드라이브에서 STL 파일 Base64로 반환
  if (mode === "stl") {
    var fileName = (e && e.parameter && e.parameter.file) ? e.parameter.file : "";
    var userName = (e && e.parameter && e.parameter.user) ? e.parameter.user : "";
    return buildWebResponse_(getSTLData(fileName, userName), callback);
  }

  return buildWebResponse_({ success: false, msg: "지원하지 않는 mode입니다." }, callback);
}

function getSheetDataForViewer() {
  loadSheetNames();
  var ss         = SpreadsheetApp.getActiveSpreadsheet();
  var inputSheet = ss.getSheetByName(CFG.INPUT_SHEET) || ss.getSheets()[1];
  var lastRow    = inputSheet.getLastRow();
  if (lastRow < 2) return { rows: [] };

  var rows   = inputSheet.getRange(2, 1, lastRow - 1, CFG.COL_LABEL).getValues();
  var result = [];

  for (var i = 0; i < rows.length; i++) {
    var r        = rows[i];
    var userName = String(r[CFG.COL_USER    - 1] || "").trim();
    var printer  = String(r[CFG.COL_PRINTER - 1] || "").trim();
    if (!userName && !printer) continue;

    var dateVal = r[CFG.COL_DATE - 1];
    var ds = "";
    if (dateVal) {
      var tmp = String(Math.floor(Number(dateVal))).replace(/[^0-9]/g, "");
      ds = tmp.length === 8 ? tmp : String(dateVal).replace(/[^0-9]/g, "").substring(0, 8);
    }

    var deleted = (r[CFG.COL_VISIBLE - 1] === true ||
                   String(r[CFG.COL_VISIBLE - 1]).toUpperCase() === "TRUE");

    result.push({
      user:     userName,
      printer:  printer,
      date:     ds,
      resStart: fmtCellTime(r[CFG.COL_RES_START - 1]),
      resDur:   fmtCellDur (r[CFG.COL_RES_DUR   - 1]),
      item:     String(r[CFG.COL_ITEM   - 1] || "").trim(),
      reason:   String(r[CFG.COL_REASON - 1] || "").trim(),
      actStart: fmtCellTime(r[CFG.COL_ACT_START - 1]),
      actDur:   fmtCellDur (r[CFG.COL_ACT_DUR   - 1]),
      file:     String(r[CFG.COL_FILE   - 1] || "").trim(),   // J열 출력파일명
      status:   String(r[CFG.COL_STATUS - 1] || "").trim(),
      memo:     String(r[CFG.COL_MEMO   - 1] || "").trim(),
      label:    String(r[CFG.COL_LABEL  - 1] || "").trim(),   // O열 자동생성 레이블
      deleted:  deleted
    });
  }
  return { rows: result, updatedAt: new Date().toISOString() };
}

// ══════════════════════════════════════════════════════════
// 구글 드라이브에서 STL 파일 로드
// ══════════════════════════════════════════════════════════
function findFileInFolder_(folder, fileName) {
  var files = folder.getFilesByName(fileName);
  if (files.hasNext()) return files.next();
  // 대소문자 무시 검색
  var lowerName = String(fileName || "").trim().toLowerCase();
  var allFiles  = folder.getFiles();
  while (allFiles.hasNext()) {
    var candidate = allFiles.next();
    if (String(candidate.getName()).trim().toLowerCase() === lowerName) return candidate;
  }
  return null;
}

function getSTLData(fileName, userName) {
  try {
    fileName = String(fileName || "").trim();
    userName = String(userName || "").trim();
    if (!fileName) return { success: false, msg: "파일명이 없습니다." };

    var rootFolder   = DriveApp.getFolderById(STL_FOLDER_ID);
    var searchFolder = rootFolder;

    if (userName) {
      var userFolders = rootFolder.getFoldersByName(userName);
      if (!userFolders.hasNext())
        return { success: false, msg: userName + " 사용자의 폴더를 찾을 수 없습니다." };
      searchFolder = userFolders.next();
    }

    var file = findFileInFolder_(searchFolder, fileName);
    if (!file)
      return { success: false, msg: (userName ? userName + " 폴더에서 " : "") + "파일을 찾을 수 없습니다: " + fileName };

    var base64 = Utilities.base64Encode(file.getBlob().getBytes());
    return { success: true, data: base64, name: file.getName() };
  } catch (err) {
    return { success: false, msg: "STL 로드 에러: " + err.toString() };
  }
}

// ══════════════════════════════════════════════════════════
// 트리거 등록 — ① 초기 세팅 후 또는 ④ 메뉴에서 실행
// ══════════════════════════════════════════════════════════
function installTriggers() {
  var ss = SpreadsheetApp.getActiveSpreadsheet();
  // 기존 트리거 전부 제거 후 재등록
  ScriptApp.getProjectTriggers().forEach(function(t) { ScriptApp.deleteTrigger(t); });
  ScriptApp.newTrigger("onEdit").forSpreadsheet(ss).onEdit().create();
  Logger.log("트리거 등록 완료: onEdit");
}

// ══════════════════════════════════════════════════════════
// 메뉴
// ══════════════════════════════════════════════════════════
function onOpen() {
  loadSheetNames();
  SpreadsheetApp.getUi()
    .createMenu("🖨 프린터 예약")
    .addItem("① O열 전체 갱신 (수동)",   "refreshAllLabels")
    .addSeparator()
    .addItem("② 트리거 재등록",           "installTriggers")
    .addToUi();
}

// 🔐 최초 1회 권한 승인 (드라이브 접근 허용)
function forceAuth() {
  var folder = DriveApp.getFolderById(STL_FOLDER_ID);
  Logger.log("드라이브 연결 성공: " + folder.getName());
}