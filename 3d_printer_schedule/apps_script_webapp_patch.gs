// Apps Script patch for printer_calendar_view.html
// Paste these functions into your existing Apps Script project.

// Set the Drive folder ID where STL files are stored.
if (typeof CFG !== "undefined") {
  CFG.DRIVE_FOLDER_ID = CFG.DRIVE_FOLDER_ID || "PUT_YOUR_DRIVE_FOLDER_ID_HERE";
}

function buildViewerTypeLabel(row, type) {
  var userName = String(row[CFG.COL_USER - 1] || "").trim();
  var item     = String(row[CFG.COL_ITEM - 1] || "").trim();
  var fileName = String(row[CFG.COL_FILE - 1] || "").trim();
  var parts = [];
  if (userName) parts.push(userName);
  if (item) parts.push(item);
  if (fileName) parts.push(fileName);
  if (!parts.length) parts.push(type === "res" ? "예약 일정" : "실사용 일정");
  return parts.join(" / ");
}

function splitLabelByType(labelText, fallbackRow) {
  var text = String(labelText || "").trim();
  var lines = text ? text.split(/\r?\n/).map(function(line) { return String(line).trim(); }).filter(String) : [];
  var resLine = "";
  var actLine = "";

  for (var i = 0; i < lines.length; i++) {
    var line = lines[i];
    if (!resLine && line.indexOf("[예약]") !== -1) resLine = line.replace(/^[^\[]*\[예약\]\s*/, "").trim();
    if (!actLine && line.indexOf("[실사용]") !== -1) actLine = line.replace(/^[^\[]*\[실사용\]\s*/, "").trim();
  }

  if (!resLine && lines.length) resLine = lines[0];
  if (!actLine && lines.length > 1) actLine = lines[1];
  if (!resLine) resLine = buildViewerTypeLabel(fallbackRow, "res");
  if (!actLine) actLine = buildViewerTypeLabel(fallbackRow, "act");

  return { resLabel: resLine, actLabel: actLine };
}

function doGet(e) {
  var mode = (e && e.parameter && e.parameter.mode) ? e.parameter.mode : "data";

  if (mode === "ts") {
    var ts = PropertiesService.getScriptProperties().getProperty("LAST_EDIT_TS") || "0";
    return ContentService.createTextOutput(JSON.stringify({ ts: ts })).setMimeType(ContentService.MimeType.JSON);
  }

  if (mode === "stl") {
    var fileName = (e && e.parameter && e.parameter.file) ? e.parameter.file : "";
    return ContentService
      .createTextOutput(JSON.stringify(getSTLData(fileName)))
      .setMimeType(ContentService.MimeType.JSON);
  }

  return ContentService
    .createTextOutput(JSON.stringify(getSheetDataForViewer()))
    .setMimeType(ContentService.MimeType.JSON);
}

function getSheetDataForViewer() {
  loadSheetNames();
  var ss         = SpreadsheetApp.getActiveSpreadsheet();
  var inputSheet = ss.getSheetByName(CFG.INPUT_SHEET) || ss.getSheets()[1];
  var lastRow    = inputSheet.getLastRow();
  if (lastRow < 2) return { rows: [] };

  var rows = inputSheet.getRange(2, 1, lastRow - 1, CFG.COL_LABEL).getValues();
  var result = [];

  for (var i = 0; i < rows.length; i++) {
    var r        = rows[i];
    var userName = String(r[CFG.COL_USER - 1]).trim();
    var printer  = String(r[CFG.COL_PRINTER - 1]).trim();
    if (!userName && !printer) continue;

    var dateVal = r[CFG.COL_DATE - 1];
    var ds = "";
    if (dateVal) {
      var tmp = String(Math.floor(Number(dateVal))).replace(/[^0-9]/g, "");
      ds = tmp.length === 8 ? tmp : String(dateVal).replace(/[^0-9]/g, "").substring(0, 8);
    }

    var deleted = (r[CFG.COL_VISIBLE - 1] === true ||
                   String(r[CFG.COL_VISIBLE - 1]).toUpperCase() === "TRUE");

    var rawLabel = String(r[CFG.COL_LABEL - 1] || "").trim();
    var splitLabel = splitLabelByType(rawLabel, r);

    result.push({
      user:      userName,
      printer:   printer,
      date:      ds,
      resStart:  fmtCellTime(r[CFG.COL_RES_START - 1]),
      resDur:    fmtCellDur(r[CFG.COL_RES_DUR - 1]),
      item:      String(r[CFG.COL_ITEM - 1]).trim(),
      reason:    String(r[CFG.COL_REASON - 1]).trim(),
      actStart:  fmtCellTime(r[CFG.COL_ACT_START - 1]),
      actDur:    fmtCellDur(r[CFG.COL_ACT_DUR - 1]),
      file:      String(r[CFG.COL_FILE - 1]).trim(),
      status:    String(r[CFG.COL_STATUS - 1]).trim(),
      memo:      String(r[CFG.COL_MEMO - 1]).trim(),
      label:     rawLabel,
      resLabel:  splitLabel.resLabel,
      actLabel:  splitLabel.actLabel,
      deleted:   deleted
    });
  }

  return { rows: result, updatedAt: new Date().toISOString() };
}

function getSTLData(fileName) {
  try {
    fileName = String(fileName || "").trim();
    if (!fileName) return { success: false, msg: "파일명이 없습니다." };
    if (!CFG.DRIVE_FOLDER_ID || CFG.DRIVE_FOLDER_ID === "PUT_YOUR_DRIVE_FOLDER_ID_HERE") {
      return { success: false, msg: "CFG.DRIVE_FOLDER_ID를 설정하세요." };
    }

    var folder = DriveApp.getFolderById(CFG.DRIVE_FOLDER_ID);
    var files = folder.getFilesByName(fileName);
    if (!files.hasNext()) {
      var lowerName = fileName.toLowerCase();
      var allFiles = folder.getFiles();
      while (allFiles.hasNext()) {
        var candidate = allFiles.next();
        if (String(candidate.getName()).toLowerCase() === lowerName) {
          return {
            success: true,
            data: Utilities.base64Encode(candidate.getBlob().getBytes()),
            name: candidate.getName()
          };
        }
      }
      return { success: false, msg: "드라이브 폴더에서 STL 파일을 찾지 못했습니다: " + fileName };
    }

    var file = files.next();
    return {
      success: true,
      data: Utilities.base64Encode(file.getBlob().getBytes()),
      name: file.getName()
    };
  } catch (err) {
    return { success: false, msg: "STL 로드 오류: " + err.toString() };
  }
}
