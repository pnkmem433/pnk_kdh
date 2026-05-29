import { BadRequestException, Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import * as fs from 'fs';
import * as path from 'path';
import { firmwareDownloadDto } from 'src/dto/firmwareDownload/download.dto';
import { Repository } from 'typeorm';
import { Project } from '../../entites/project.entity';
import { ProjectVersion } from '../../entites/version.entity';
import { buildOtaLogContext, otaError, otaLog, parseFirmwareDownloadInput } from './ota-log.util';

@Injectable()
export class DownloadService {
  constructor(
    @InjectRepository(ProjectVersion)
    private readonly projectVersionRepository: Repository<ProjectVersion>,
    @InjectRepository(Project)
    private readonly projectRepository: Repository<Project>,
  ) {}

  private normalizeChipType(rawChipType: string) {
    return String(rawChipType ?? '').trim().toLowerCase();
  }

  private normalizeFamily(rawFamily: string) {
    return String(rawFamily ?? '').trim().toLowerCase();
  }

  private normalizeBootSourceHint(rawHint: string | undefined) {
    return String(rawHint ?? '').trim().toLowerCase();
  }

  private resolveBinFilePath(binFilePath: string, label: string) {
    otaLog({}, `file-check:${label}:${binFilePath}`);
    if (binFilePath && fs.existsSync(binFilePath)) {
      return binFilePath;
    }

    const fallbackPath = binFilePath
      ? path.join(process.cwd(), 'uploads', path.basename(binFilePath))
      : '';
    if (fallbackPath && fs.existsSync(fallbackPath)) {
      return fallbackPath;
    }

    throw new NotFoundException('bin file not found.');
  }

  private resolveFirstExistingPath(candidates: string[]) {
    for (const candidate of candidates) {
      if (!candidate) {
        continue;
      }
      if (fs.existsSync(candidate)) {
        return candidate;
      }
    }
    throw new NotFoundException('bin file not found.');
  }

  private migrationSafebootPath(chipType: string) {
    const normalizedChipType = this.normalizeChipType(chipType);
    const fileMap: Record<string, string> = {
      esp8685: path.join(process.cwd(), 'migration-binaries', 'esp8685_tasmota_partition_bridge.bin'),
      esp32: path.join(process.cwd(), 'migration-binaries', 'esp8685_tasmota_partition_bridge.bin'),
    };
    return fileMap[normalizedChipType] ?? '';
  }

  private realSafebootPath(chipType: string) {
    const normalizedChipType = this.normalizeChipType(chipType);
    const fileMap: Record<string, string> = {
      esp8685: path.join(process.cwd(), 'migration-binaries', 'esp8685_tasmota_migration_safeboot.bin'),
      esp32: path.join(process.cwd(), 'migration-binaries', 'esp8685_tasmota_migration_safeboot.bin'),
    };
    return fileMap[normalizedChipType] ?? '';
  }

  private esp02sTasmotaMinimalCandidates() {
    const envPath = String(process.env.ESP02S_TASMOTA_MINIMAL_PATH ?? '').trim();
    return [
      envPath,
      path.join(process.cwd(), 'uploads', 'tasmota-minimal.bin.gz'),
      path.join(process.cwd(), 'migration-binaries', 'tasmota-minimal.bin.gz'),
    ].filter(Boolean);
  }

  async downloadEsp02sTasmotaMinimal() {
    const binFilePath = this.resolveFirstExistingPath(this.esp02sTasmotaMinimalCandidates());
    if (!binFilePath.toLowerCase().endsWith('.bin.gz')) {
      throw new NotFoundException('esp02s Tasmota minimal gzip file not found.');
    }
    return { binFilePath };
  }

  async downloadMigrationSafeboot(chipType: string) {
    const binFilePath = this.migrationSafebootPath(chipType);
    return { binFilePath: this.resolveBinFilePath(binFilePath, `migration-safeboot-${chipType}`) };
  }

  async downloadRealSafeboot(chipType: string) {
    const binFilePath = this.realSafebootPath(chipType);
    return { binFilePath: this.resolveBinFilePath(binFilePath, `migration-real-safeboot-${chipType}`) };
  }

  async downloadMigrationMain(projectId: number, chipType: string) {
    const normalizedChipType = this.normalizeChipType(chipType);
    const targetFamily = 'custom';
    const project = await this.projectRepository.findOneBy({ id: projectId });
    if (!project) {
      throw new NotFoundException('project not found.');
    }

    const versions = await this.projectVersionRepository.find({
      where: { project: { id: project.id }, isActive: true },
      order: { createdAt: 'DESC', id: 'DESC' },
    });

    for (const version of versions) {
      const versionChipType = this.normalizeChipType(String(version.chipType ?? ''));
      const versionFamily = this.normalizeFamily(String(version.firmwareFamily ?? ''));
      if (versionChipType !== normalizedChipType || versionFamily !== targetFamily) {
        continue;
      }

      const binFilePath = this.resolveBinFilePath(version.binFile, `migration-main-${version.id}`);
      otaLog({ projectId, chipType: normalizedChipType }, `migration-main:selected:${version.id}`);
      return { binFilePath };
    }

    throw new NotFoundException('matching migration main firmware not found.');
  }

  async downloadFirmware(input: firmwareDownloadDto | string) {
    const parsedInput = parseFirmwareDownloadInput(input);
    const context = buildOtaLogContext(parsedInput);
    otaLog(context, 'download-start');

    let dto: unknown = input;
    if (typeof input === 'string') {
      try {
        dto = JSON.parse(input.replace(/\t/g, '').trim());
      } catch (error) {
        otaError(context, 'request-json-parse-failed', error);
        throw new BadRequestException('request JSON parse failed.');
      }
    }

    const request = dto as firmwareDownloadDto;
    const projectId = Number(request?.projectId);
    const chipType = this.normalizeChipType(String(request?.chipType ?? ''));
    const currentFirmwareFamily = this.normalizeFamily(String(request?.currentFirmwareFamily ?? ''));
    const currentVersion = Number(request?.currentVersion);
    const bootSourceHint = this.normalizeBootSourceHint(request?.bootSourceHint);
    const requestContext = {
      ...context,
      projectId,
      chipType,
      currentFirmwareFamily,
      currentVersion,
      bootSourceHint,
    };

    if (!Number.isInteger(projectId) || projectId <= 0) {
      throw new BadRequestException('projectId is invalid.');
    }
    if (!chipType) {
      throw new BadRequestException('chipType is required.');
    }
    if (!currentFirmwareFamily) {
      throw new BadRequestException('currentFirmwareFamily is required.');
    }
    if (!Number.isInteger(currentVersion) || currentVersion < 0) {
      throw new BadRequestException('currentVersion is invalid.');
    }

    const project = await this.projectRepository.findOneBy({ id: projectId });
    if (!project) {
      throw new NotFoundException('project not found.');
    }

    const versions = await this.projectVersionRepository.find({
      where: { project: { id: project.id }, isActive: true },
      order: { createdAt: 'DESC', id: 'DESC' },
    });

    if (!versions.length) {
      throw new NotFoundException('project version not found.');
    }

    const chipVersions = versions.filter((version) => {
      const versionChipType = this.normalizeChipType(String(version.chipType ?? ''));
      return versionChipType === chipType;
    });

    if (!chipVersions.length) {
      throw new NotFoundException('matching chipType version not found.');
    }

    const latestChipVersion = chipVersions[0];
    const latestChipFamily = this.normalizeFamily(String(latestChipVersion.firmwareFamily ?? ''));
    const latestChipVersionNumber = Number(latestChipVersion.versionNumber ?? 0);

    const sameFamilyVersions = chipVersions.filter((version) => {
      const versionFamily = this.normalizeFamily(String(version.firmwareFamily ?? ''));
      return versionFamily === currentFirmwareFamily;
    });
    const exactCurrentTrackVersion = sameFamilyVersions.find(
      (version) => Number(version.versionNumber ?? 0) === currentVersion,
    );

    otaLog(
      requestContext,
      `latest-chip:id=${latestChipVersion.id},family=${latestChipFamily},version=${latestChipVersionNumber}`,
    );

    if (latestChipFamily !== currentFirmwareFamily) {
      if (bootSourceHint === 'external_install' && exactCurrentTrackVersion) {
        otaLog(requestContext, 'cross-family-suppressed-after-external-install');
        throw new BadRequestException('no newer firmware available in the same family.');
      }

      const binFilePath = this.resolveBinFilePath(latestChipVersion.binFile, `candidate-${latestChipVersion.id}`);
      otaLog(requestContext, `cross-family-switch:${currentFirmwareFamily}->${latestChipFamily}`);
      return {
        binFilePath,
        firmwareFamily: latestChipVersion.firmwareFamily,
        message: 'firmware family changed.',
      };
    }

    if (latestChipVersionNumber > currentVersion) {
      const binFilePath = this.resolveBinFilePath(latestChipVersion.binFile, `candidate-${latestChipVersion.id}`);
      otaLog(requestContext, `same-family-upgrade:${currentVersion}->${latestChipVersionNumber}`);
      return {
        binFilePath,
        firmwareFamily: latestChipVersion.firmwareFamily,
        message: 'newer firmware in the same family found.',
      };
    }

    otaLog(requestContext, 'same-family-no-update');
    throw new BadRequestException('no newer firmware available in the same family.');
  }
}
