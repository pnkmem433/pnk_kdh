import { Injectable, ConflictException, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { v4 as uuidv4 } from 'uuid';
import { Project } from '../../entites/project.entity';
import { User } from 'src/entites/user.entity';
import { ProjectCreateDto } from 'src/dto/project/create.dto';
import { ProjectUpdateNameDto } from 'src/dto/project/update-name.dto';

@Injectable()
export class ProjectsService {
  constructor(
    @InjectRepository(Project)
    private projectsRepository: Repository<Project>,
  ) { }

  async getUserProjects(user: User): Promise<{ id: number, name: string, uuid: string }[]> {
    const projects = await this.projectsRepository.find({ where: { userSeq: { seq: user.seq } } });
    return projects.map(project => ({ id: project.id, uuid: project.uuid, name: project.name }));
  }


  async create(user: User, createProject: ProjectCreateDto): Promise<{ id: number, name: string, uuid: string }> {
    const existingProject = await this.projectsRepository.findOne({ where: { name: createProject.newName, userSeq: { seq: user.seq } } });

    if (existingProject) {
      throw new ConflictException('이미 존재하는 프로젝트 이름입니다.');
    }

    const newProject = this.projectsRepository.create({ uuid: uuidv4(), name: createProject.newName, userSeq: { seq: user.seq } });
    this.projectsRepository.save(newProject)
    return { id: newProject.id, name: newProject.name, uuid: newProject.uuid };
  }

  async updateName(user: User, projectUpdateNameDto: ProjectUpdateNameDto): Promise<{ id: number, name: string, uuid: string }> {
    const project = await this.projectsRepository.findOne({ where: { id: projectUpdateNameDto.id, userSeq: { seq: user.seq } } });

    if (!project)
      throw new NotFoundException('프로젝트를 찾을 수 없습니다.');

    const existingProject = await this.projectsRepository.findOne({ where: { name: projectUpdateNameDto.newName, userSeq: { seq: user.seq } } });

    if (existingProject && existingProject.id !== projectUpdateNameDto.id)
      throw new ConflictException('이미 존재하는 프로젝트 이름입니다.');

    project.name = projectUpdateNameDto.newName;

    await this.projectsRepository.save(project);
    return { id: project.id, name: project.name, uuid: project.uuid };
  }
}
