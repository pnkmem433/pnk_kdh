import { BadRequestException, Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { ProjectVersion } from '../../entites/version.entity';
import { Project } from '../../entites/project.entity';
import { VersionSearchDto } from 'src/dto/version/search.dto';
import { User } from 'src/entites/user.entity';
import { VersionCreateDto } from 'src/dto/version/create.dto';
import { extname } from 'path';
import * as path from 'path';
import * as fs from 'fs';

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
      throw new NotFoundException('프로젝트를 찾을 수 없습니다.');
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
        `입력한 버전 번호는 같은 칩/계열의 최신 버전(${latestSameTrack.versionNumber})보다 커야 합니다.`,
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
    const uploadsDir = path.join(__dirname, '../../uploads');
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
      console.log(`File moved successfully to ${newFilePath}`);
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
      throw new NotFoundException('프로젝트를 찾을 수 없습니다.');
    }

    const versions = await this.projectVersionRepository.find({
      where: {
        isActive: true,
        project: { id: versionSearchDto.projectId, userSeq: { seq: user.seq } },
      },
      order: { createdAt: 'DESC', id: 'DESC' },
    });

    if (versions.length <= 1) {
      throw new NotFoundException('해당 프로젝트에서 비활성화할 최신 버전이 없습니다.');
    }

    versions[0].isActive = false;
    await this.projectVersionRepository.save(versions[0]);

    return {
      message: '최신 버전이 성공적으로 비활성화되었습니다.',
    };
  }
}
