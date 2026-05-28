import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ProjectVersion } from '../../entites/version.entity';
import { ProjectVersionService } from './versions.service';
import { ProjectVersionController } from './versions.controller';
import { Project } from '../../entites/project.entity';

@Module({
  imports: [TypeOrmModule.forFeature([ProjectVersion, Project])],
  providers: [ProjectVersionService],
  controllers: [ProjectVersionController],
  exports: [ProjectVersionService],
})
export class ProjectVersionModule {}
