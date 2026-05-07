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
  ) {}

  async downloadFirmware(input: firmwareDownloadDto | string) {
    console.log('========== [OTA] downloadFirmware START ==========');
    console.log('[OTA] input type =', typeof input);

    let dto: any = input;

    if (typeof input === 'string') {
      console.log('[OTA] raw string length =', input.length);
      const sanitizedRaw = input.replace(/\t/g, '').trim();
      console.log('[OTA] sanitized raw length =', sanitizedRaw.length);

      try {
        dto = JSON.parse(sanitizedRaw);
      } catch (e) {
        console.error('[OTA][ERROR] JSON parse failed:', e);
        throw new BadRequestException('요청 JSON 파싱에 실패했습니다.');
      }
    }

    const projectId = Number(dto?.projectId);
    const chipType = String(dto?.chipType ?? '').trim();
    const currentFirmwareFamily = String(dto?.currentFirmwareFamily ?? '').trim();
    const currentVersion = Number(dto?.currentVersion);

    console.log('[OTA] normalized projectId =', projectId);
    console.log('[OTA] normalized chipType =', chipType);
    console.log('[OTA] normalized currentFirmwareFamily =', currentFirmwareFamily);
    console.log('[OTA] normalized currentVersion =', currentVersion);

    if (!Number.isInteger(projectId) || projectId <= 0) {
      throw new BadRequestException('projectId가 올바르지 않습니다.');
    }
    if (!chipType) {
      throw new BadRequestException('chipType이 비어 있습니다.');
    }
    if (!currentFirmwareFamily) {
      throw new BadRequestException('currentFirmwareFamily가 비어 있습니다.');
    }
    if (!Number.isInteger(currentVersion) || currentVersion < 0) {
      throw new BadRequestException('currentVersion이 올바르지 않습니다.');
    }

    const project = await this.projectRepository.findOneBy({ id: projectId });
    if (!project) {
      throw new NotFoundException('프로젝트를 찾을 수 없습니다.');
    }

    const versions = await this.projectVersionRepository.find({
      where: { project: { id: project.id }, isActive: true },
      order: { createdAt: 'DESC', id: 'DESC' },
    });

    if (!versions.length) {
      throw new NotFoundException('프로젝트 버전을 찾을 수 없습니다.');
    }

    let foundSameChip = false;
    let foundSameTrack = false;

    for (const version of versions) {
      if (version.chipType !== chipType) {
        continue;
      }

      foundSameChip = true;

      if (version.firmwareFamily !== currentFirmwareFamily) {
        if (!version.binFile || !fs.existsSync(version.binFile)) {
          throw new NotFoundException('bin 파일을 찾을 수 없습니다.');
        }

        console.log('[OTA] firmware family changed -> OTA immediately');
        console.log('========== [OTA] downloadFirmware END ==========');
        return {
          binFilePath: version.binFile,
          message: '다른 펌웨어 계열이므로 버전 비교 없이 업데이트를 진행합니다.',
        };
      }

      foundSameTrack = true;

      if (Number(version.versionNumber) > currentVersion) {
        if (!version.binFile || !fs.existsSync(version.binFile)) {
          throw new NotFoundException('bin 파일을 찾을 수 없습니다.');
        }

        console.log('[OTA] same firmware family and newer version found');
        console.log('========== [OTA] downloadFirmware END ==========');
        return {
          binFilePath: version.binFile,
          message: '상위 버전이 있어 업데이트를 진행합니다.',
        };
      }
    }

    if (!foundSameChip) {
      throw new NotFoundException('해당 칩 종류에 맞는 버전을 찾을 수 없습니다.');
    }

    if (foundSameTrack) {
      throw new BadRequestException('현재 버전보다 높은 같은 계열의 업데이트가 없습니다.');
    }

    throw new NotFoundException('해당 칩 종류에 맞는 업그레이드 후보를 찾을 수 없습니다.');
  }
}
