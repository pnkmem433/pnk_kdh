import type { firmwareDownloadDto } from 'src/dto/firmwareDownload/download.dto';

type RequestHeaders = Record<string, string | string[] | undefined>;

type OtaLogContext = {
  projectId?: number;
  chipType?: string;
  currentFirmwareFamily?: string;
  currentVersion?: number;
  macAddress?: string;
  rawMacAddress?: string;
};

function timestamp() {
  const formatter = new Intl.DateTimeFormat('ko-KR', {
    timeZone: 'Asia/Seoul',
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    hour12: false,
  });
  const parts = formatter.formatToParts(new Date());
  const valueOf = (type: string) => parts.find((part) => part.type === type)?.value ?? '00';
  return `${valueOf('year')}-${valueOf('month')}-${valueOf('day')} ${valueOf('hour')}:${valueOf('minute')}:${valueOf('second')}`;
}

function pickHeader(headers: RequestHeaders | undefined, key: string) {
  if (!headers) {
    return '';
  }
  const matchedKey = Object.keys(headers).find((headerKey) => headerKey.toLowerCase() === key.toLowerCase());
  if (!matchedKey) {
    return '';
  }
  const value = headers[matchedKey];
  if (Array.isArray(value)) {
    return String(value[0] ?? '').trim();
  }
  return String(value ?? '').trim();
}

export function normalizeMacAddress(rawValue: unknown) {
  const normalized = String(rawValue ?? '')
    .trim()
    .replace(/[^0-9a-fA-F]/g, '')
    .toUpperCase();
  return normalized.length === 12 ? normalized : '';
}

export function parseFirmwareDownloadInput(input: firmwareDownloadDto | string) {
  if (typeof input !== 'string') {
    return input;
  }
  try {
    return JSON.parse(input.replace(/\t/g, '').trim()) as Partial<firmwareDownloadDto> & {
      macAddress?: string;
    };
  } catch {
    return {};
  }
}

export function buildOtaLogContext(
  input?: Partial<firmwareDownloadDto> & { macAddress?: string },
  headers?: RequestHeaders,
): OtaLogContext {
  const headerMac =
    pickHeader(headers, 'x-esp32-sta-mac') ||
    pickHeader(headers, 'x-esp8266-sta-mac') ||
    pickHeader(headers, 'x-device-mac') ||
    pickHeader(headers, 'x-mac-address');
  const rawMacAddress = String(input?.macAddress ?? headerMac ?? '').trim();
  const macAddress = normalizeMacAddress(rawMacAddress);
  const currentVersion = Number(input?.currentVersion);

  return {
    projectId: Number(input?.projectId ?? 0) || undefined,
    chipType: String(input?.chipType ?? '').trim().toLowerCase() || undefined,
    currentFirmwareFamily: String(input?.currentFirmwareFamily ?? '').trim().toLowerCase() || undefined,
    currentVersion: Number.isFinite(currentVersion) ? currentVersion : undefined,
    macAddress,
    rawMacAddress,
  };
}

export function otaLog(context: OtaLogContext, message: string) {
  const macText = context.macAddress
    ? `MAC=${context.macAddress}`
    : context.rawMacAddress
      ? `MAC=형식오류(${context.rawMacAddress})`
      : 'MAC=미확인';
  const projectText = context.projectId ? `프로젝트=${context.projectId}` : '프로젝트=미확인';
  const chipText = context.chipType ? `칩=${context.chipType}` : '칩=미확인';
  const familyText = context.currentFirmwareFamily
    ? `현재계열=${context.currentFirmwareFamily}`
    : '현재계열=미확인';
  const versionText =
    typeof context.currentVersion === 'number' && !Number.isNaN(context.currentVersion)
      ? `현재버전=v${context.currentVersion}`
      : '현재버전=미확인';

  console.log(
    `[${timestamp()}] [OTA] ${macText} | ${projectText} | ${chipText} | ${familyText} | ${versionText} | ${message}`,
  );
}

export function otaError(context: OtaLogContext, message: string, error?: unknown) {
  otaLog(context, message);
  if (error) {
    console.error(error);
  }
}
