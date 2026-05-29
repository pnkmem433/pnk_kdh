import { BadRequestException, Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import * as fs from 'fs';
import { extname } from 'path';
import * as path from 'path';
import { User } from 'src/entites/user.entity';
import { VersionCreateDto } from 'src/dto/version/create.dto';
import { VersionDeleteDto } from 'src/dto/version/delete.dto';
import { VersionSearchDto } from 'src/dto/version/search.dto';
import { Repository } from 'typeorm';
import { Project } from '../../entites/project.entity';
import { ProjectVersion } from '../../entites/version.entity';

@Injectable()
export class ProjectVersionService {
  constructor(
    @InjectRepository(ProjectVersion)
    private readonly projectVersionRepository: Repository<ProjectVersion>,
    @InjectRepository(Project)
    private readonly projectRepository: Repository<Project>,
  ) {}

  async getVersions(
    user: User,
    versionSearchDto: VersionSearchDto,
  ): Promise<
    {
      id: number;
      name: string;
      chipType: string;
      firmwareFamily: string;
      created: Date;
      isActive: boolean;
    }[]
  > {
    const versions = await this.projectVersionRepository.find({
      where: { project: { id: versionSearchDto.projectId, userSeq: { seq: user.seq } } },
      order: { createdAt: 'DESC', id: 'DESC' },
    });

    return versions.map((version) => ({
      id: version.versionNumber,
      name: version.versionName,
      chipType: version.chipType,
      firmwareFamily: version.firmwareFamily,
      created: version.createdAt,
      isActive: version.isActive,
    }));
  }

  async createVersion(user: User, createVersionDto: VersionCreateDto, binFile: Express.Multer.File) {
    const project = await this.projectRepository.findOneBy({
      id: createVersionDto.projectId,
      userSeq: { seq: user.seq },
    });

    if (!project) {
      throw new NotFoundException('project not found.');
    }

    const latestSameTrack = await this.projectVersionRepository.findOne({
      where: {
        project: { id: createVersionDto.projectId, userSeq: { seq: user.seq } },
        chipType: createVersionDto.chipType,
        firmwareFamily: createVersionDto.firmwareFamily,
      },
      order: { versionNumber: 'DESC', id: 'DESC' },
    });

    if (
      latestSameTrack &&
      Number(latestSameTrack.versionNumber) >= Number(createVersionDto.versionNumber)
    ) {
      throw new BadRequestException(
        `versionNumber must be greater than the latest ${createVersionDto.chipType}/${createVersionDto.firmwareFamily} version (${latestSameTrack.versionNumber}).`,
      );
    }

    const newVersion = this.projectVersionRepository.create({
      project,
      versionNumber: createVersionDto.versionNumber,
      versionName: createVersionDto.versionName,
      chipType: createVersionDto.chipType,
      firmwareFamily: createVersionDto.firmwareFamily,
    });

    await this.projectVersionRepository.save(newVersion);

    const newFileName = `${newVersion.id}${extname(binFile.originalname)}`;
    const uploadsDir = path.join(process.cwd(), 'uploads');
    const newFilePath = path.join(uploadsDir, newFileName);
    const tempPath = binFile.path;

    if (!fs.existsSync(uploadsDir)) {
      fs.mkdirSync(uploadsDir, { recursive: true });
    }

    if (!fs.existsSync(tempPath)) {
      throw new Error(`Temporary file not found: ${tempPath}`);
    }

    try {
      fs.copyFileSync(tempPath, newFilePath);
      fs.unlinkSync(tempPath);
    } catch (error) {
      throw new Error(`Error moving file: ${error.message}`);
    }

    newVersion.binFile = newFilePath;
    await this.projectVersionRepository.save(newVersion);

    return {
      id: newVersion.versionNumber,
      name: newVersion.versionName,
      chipType: newVersion.chipType,
      firmwareFamily: newVersion.firmwareFamily,
      created: newVersion.createdAt,
      isActive: newVersion.isActive,
    };
  }

  async deleteLatestVersionByProjectId(
    user: User,
    versionSearchDto: VersionSearchDto,
  ): Promise<{ message: string }> {
    const project = await this.projectRepository.findOneBy({
      id: versionSearchDto.projectId,
      userSeq: { seq: user.seq },
    });

    if (!project) {
      throw new NotFoundException('project not found.');
    }

    const versions = await this.projectVersionRepository.find({
      where: {
        isActive: true,
        project: { id: versionSearchDto.projectId, userSeq: { seq: user.seq } },
      },
      order: { createdAt: 'DESC', id: 'DESC' },
    });

    if (versions.length <= 1) {
      throw new NotFoundException('no latest active version to deactivate.');
    }

    versions[0].isActive = false;
    await this.projectVersionRepository.save(versions[0]);

    return {
      message: 'latest version deactivated.',
    };
  }

  async hardDeleteVersionRow(
    user: User,
    versionDeleteDto: VersionDeleteDto,
  ): Promise<{ message: string; deletedRows: number }> {
    const project = await this.projectRepository.findOneBy({
      id: versionDeleteDto.projectId,
      userSeq: { seq: user.seq },
    });

    if (!project) {
      throw new NotFoundException('project not found.');
    }

    const rows = await this.projectVersionRepository.find({
      where: {
        project: { id: versionDeleteDto.projectId, userSeq: { seq: user.seq } },
        versionNumber: versionDeleteDto.versionNumber,
        chipType: versionDeleteDto.chipType,
        firmwareFamily: versionDeleteDto.firmwareFamily,
      },
      relations: ['project'],
      order: { createdAt: 'DESC', id: 'DESC' },
    });

    if (!rows.length) {
      throw new NotFoundException('matching version row not found.');
    }

    for (const row of rows) {
      if (row.binFile && fs.existsSync(row.binFile)) {
        try {
          fs.unlinkSync(row.binFile);
        } catch (error) {
          console.warn(`[versions] failed to remove file ${row.binFile}`, error);
        }
      }
      await this.projectVersionRepository.remove(row);
    }

    return {
      message: `deleted ${rows.length} row(s).`,
      deletedRows: rows.length,
    };
  }
}
