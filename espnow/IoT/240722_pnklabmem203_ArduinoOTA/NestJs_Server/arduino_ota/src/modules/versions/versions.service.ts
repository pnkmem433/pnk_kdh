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
  ) { }

  async getVersions(user: User, versionSearchDto: VersionSearchDto): Promise<{ id: number, name: string, created: Date, isActive: boolean }[]> {
    const versions = await this.projectVersionRepository.find({ where: { project: { id: versionSearchDto.projectId, userSeq: { seq: user.seq } } } });
    return versions.map((version) => { return { id: version.versionNumber, name: version.versionName, created: version.createdAt, isActive: version.isActive }; });
  }

  async createVersion(user: User, createVersionDto: VersionCreateDto, binFile: Express.Multer.File) {
    const project = await this.projectRepository.findOneBy({ id: createVersionDto.projectId, userSeq: { seq: user.seq } });

    if (!project)
      throw new NotFoundException('프로젝트를 찾을 수 없습니다.');

    const versions = await this.projectVersionRepository.find({ where: { project: { id: createVersionDto.projectId, userSeq: { seq: user.seq } } } });

    if (versions.length > 0 && versions[versions.length - 1].versionNumber >= createVersionDto.versionNumber)
      throw new BadRequestException(`입력한 버전 번호는 최대버전(${versions[versions.length - 1].versionNumber})보다 커야 합니다.`);


    const versionNumber = createVersionDto.versionNumber;
    const versionName = createVersionDto.versionName;

    const newVersion = this.projectVersionRepository.create({
      project: project,
      versionNumber,
      versionName,
    });


    await this.projectVersionRepository.save(newVersion);

    const newFileName = `${newVersion.id}${extname(binFile.originalname)}`;
    const uploadsDir = path.join(__dirname, '../../uploads');
    const newFilePath = path.join(uploadsDir, newFileName);
    const tempPath = binFile.path;

    if (!fs.existsSync(uploadsDir)) {
      fs.mkdirSync(uploadsDir, { recursive: true });
    }

    if (fs.existsSync(tempPath)) {
      try {
        fs.copyFileSync(tempPath, newFilePath);
        fs.unlinkSync(tempPath);
        console.log(`File moved successfully to ${newFilePath}`);
      } catch (error) {
        throw new Error(`Error moving file: ${error.message}`);
      }
    } else {
      throw new Error(`Temporary file not found: ${tempPath}`);
    }

    newVersion.binFile = newFilePath;

    await this.projectVersionRepository.save(newVersion);

    return { id: newVersion.versionNumber, name: newVersion.versionName, created: newVersion.createdAt, isActive: newVersion.isActive };
  }

  async deleteLatestVersionByProjectId(user: User, versionSearchDto: VersionSearchDto): Promise<{ message: string }> {
    const project = await this.projectRepository.findOneBy({ id: versionSearchDto.projectId, userSeq: { seq: user.seq } });
    if (!project) {
      throw new NotFoundException('프로젝트를 찾을 수 없습니다.');
    }

    const versions = await this.projectVersionRepository.find({ where: { isActive: true, project: { id: versionSearchDto.projectId, userSeq: { seq: user.seq } } } });

    if (versions.length > 1) {
      versions[versions.length - 1].isActive = false;
      await this.projectVersionRepository.save(versions[versions.length - 1]);

      return {
        message: '최신 버전이 성공적으로 비활성화되었습니다.'
      }
    } else {
      throw new NotFoundException('해당 프로젝트에 대한 삭제할 수 있는 버전이 없습니다.');
    }
  }

}
