import { Injectable, NotFoundException, BadRequestException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { ProjectVersion } from '../../entites/version.entity';
import { Project } from '../../entites/project.entity';
import * as fs from 'fs';
import { firmwareDownloadDto } from 'src/dto/firmwareDownload/download.dto';

@Injectable()
export class DownloadService {
  constructor(
    @InjectRepository(ProjectVersion)
    private readonly projectVersionRepository: Repository<ProjectVersion>,
    @InjectRepository(Project)
    private readonly projectRepository: Repository<Project>,
  ) { }

async downloadFirmware(input: firmwareDownloadDto | string) {
  console.log('========== [OTA] downloadFirmware START ==========');
  console.log('[OTA] input type =', typeof input);

  // ============================
  // ✅ 임시 땜빵: string(raw json)도 허용
  // - bodyParser.text()로 들어오면 input은 string임
  // - 여기서 sanitize + JSON.parse 해서 dto로 만든다
  // ============================
  let dto: any = input;

  if (typeof input === 'string') {
    console.log('[OTA] raw string length =', input.length);

    // 탭/앞뒤 공백 정도만 최소 정리
    const sanitizedRaw = input.replace(/\t/g, '').trim();
    console.log('[OTA] sanitized raw length =', sanitizedRaw.length);

    try {
      dto = JSON.parse(sanitizedRaw);
    } catch (e) {
      console.error('[OTA][ERROR] JSON parse failed:', e);
      throw new BadRequestException('요청 JSON 파싱에 실패했습니다.');
    }
  }

  console.log('[OTA] dto =', dto);
  console.log('[OTA] dto keys =', dto ? Object.keys(dto) : null);

  // ✅ 여기부터는 기존 로직 유지 (단, dto 기준으로 접근)
  const projectId = String(dto?.ProjectId ?? '')
    .replace(/\t/g, '')
    .trim();

  const currentVersion = Number(dto?.currentVersion);

  console.log('[OTA] normalized projectId =', projectId, 'length =', projectId.length);
  console.log('[OTA] normalized currentVersion =', currentVersion);

  if (!projectId) {
    console.error('[OTA][ERROR] ProjectId empty after normalize');
    throw new BadRequestException('ProjectId가 비어있습니다.');
  }
  if (!Number.isInteger(currentVersion) || currentVersion < 0) {
    console.error('[OTA][ERROR] invalid currentVersion =', currentVersion);
    throw new BadRequestException('currentVersion이 올바르지 않습니다.');
  }

  console.log('[OTA] lookup project by uuid =', projectId);
  const project = await this.projectRepository.findOneBy({ uuid: projectId });
  if (!project) {
    console.error('[OTA][ERROR] project not found');
    throw new NotFoundException('프로젝트를 찾을 수 없습니다.');
  }

  console.log('[OTA] project found:', { id: project.id, uuid: project.uuid });

  const projectVersion = await this.projectVersionRepository.findOne({
    where: { project: { id: project.id }, isActive: true },
    order: { versionNumber: 'DESC' },
  });

  if (!projectVersion) {
    console.error('[OTA][ERROR] projectVersion not found');
    throw new NotFoundException('프로젝트 버전을 찾을 수 없습니다.');
  }

  const latest = Number(projectVersion.versionNumber);
  console.log('[OTA] latest version (number) =', latest);

  if (!Number.isInteger(latest)) {
    console.error('[OTA][ERROR] invalid versionNumber in DB:', projectVersion.versionNumber);
    throw new BadRequestException('서버 버전 데이터가 올바르지 않습니다.');
  }

  const binFilePath = projectVersion.binFile;
  console.log('[OTA] binFilePath =', binFilePath);

  if (!fs.existsSync(binFilePath)) {
    console.error('[OTA][ERROR] bin file not exists:', binFilePath);
    throw new NotFoundException('bin 파일을 찾을 수 없습니다.');
  }

  if (currentVersion === latest) {
    console.warn('[OTA] client already latest version');
    throw new BadRequestException('현재 버전이 최신 버전입니다.');
  } else if (currentVersion < latest) {
    console.log('[OTA] update required → download allowed');
    console.log('========== [OTA] downloadFirmware END ==========');
    return { binFilePath, message: '업데이트된 버전의 bin 파일을 다운로드하십시오.' };
  } else {
    console.warn('[OTA] client version newer than server');
    console.log('========== [OTA] downloadFirmware END ==========');
    return { binFilePath, message: '현재 버전이 최신 버전보다 최신입니다.' };
  }
}


}
