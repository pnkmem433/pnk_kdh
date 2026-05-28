/**
 * DB에서 특정 프로젝트·칩·계열·버전번호 한 줄을 완전히 삭제합니다 (Swagger soft-delete 아님).
 * 업로드 폴더의 bin 파일 경로가 있으면 함께 삭제합니다.
 *
 * 사용 예 (프로젝트 10, esp02s + custom, 버전 번호 95):
 *   npx ts-node src/scripts/delete-version-row.ts --project 10 --version 95 --chip esp02s --family custom
 *
 * 또는 .env 기본값: DELETE_VERSION_PROJECT_ID, DELETE_VERSION_NUMBER, DELETE_VERSION_CHIP, DELETE_VERSION_FAMILY
 */
import * as dotenv from 'dotenv';
import * as fs from 'fs';

dotenv.config();

import dataSource from '../../data-source';
import { ProjectVersion } from '../entites/version.entity';

function parseArgs(): {
  projectId: number;
  versionNumber: number;
  chipType: string;
  firmwareFamily: string;
} {
  const argv = process.argv.slice(2);
  const get = (flag: string): string | undefined => {
    const i = argv.indexOf(flag);
    if (i >= 0 && argv[i + 1]) {
      return argv[i + 1];
    }
    return undefined;
  };

  const projectId = parseInt(
    get('--project') ?? process.env.DELETE_VERSION_PROJECT_ID ?? '10',
    10,
  );
  const versionNumber = parseInt(
    get('--version') ?? process.env.DELETE_VERSION_NUMBER ?? '0',
    10,
  );
  const chipType = get('--chip') ?? process.env.DELETE_VERSION_CHIP ?? 'esp02s';
  const firmwareFamily =
    get('--family') ?? process.env.DELETE_VERSION_FAMILY ?? 'custom';

  if (!Number.isFinite(projectId) || !Number.isFinite(versionNumber) || versionNumber <= 0) {
    throw new Error(
      'projectId와 versionNumber가 필요합니다. 예: npx ts-node src/scripts/delete-version-row.ts --project 10 --version 95 --chip esp02s --family custom',
    );
  }

  return { projectId, versionNumber, chipType, firmwareFamily };
}

async function run() {
  const { projectId, versionNumber, chipType, firmwareFamily } = parseArgs();

  await dataSource.initialize();
  const repo = dataSource.getRepository(ProjectVersion);

  const rows = await repo.find({
    where: {
      versionNumber,
      chipType,
      firmwareFamily,
      project: { id: projectId },
    },
    relations: ['project'],
  });

  if (rows.length === 0) {
    console.log(
      `[delete-version] 일치하는 행이 없습니다. project=${projectId} versionNumber=${versionNumber} chipType=${chipType} firmwareFamily=${firmwareFamily}`,
    );
    await dataSource.destroy();
    return;
  }

  for (const row of rows) {
    if (row.binFile && fs.existsSync(row.binFile)) {
      try {
        fs.unlinkSync(row.binFile);
        console.log(`[delete-version] 파일 삭제: ${row.binFile}`);
      } catch (e) {
        console.warn(`[delete-version] 파일 삭제 실패(무시): ${row.binFile}`, e);
      }
    }
    await repo.remove(row);
    console.log(
      `[delete-version] DB 행 삭제 완료: internal id=${row.id} versionNumber=${row.versionNumber} name=${row.versionName}`,
    );
  }

  await dataSource.destroy();
  console.log('[delete-version] 완료');
}

run().catch((err) => {
  console.error('[delete-version] 실패', err);
  process.exit(1);
});
