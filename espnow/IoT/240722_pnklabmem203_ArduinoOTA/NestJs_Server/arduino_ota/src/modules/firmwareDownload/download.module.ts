import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { DownloadController } from './download.controller';
import { DownloadService } from './download.service';
import { Project } from '../../entites/project.entity';
import { ProjectVersion } from '../../entites/version.entity';

@Module({
  imports: [TypeOrmModule.forFeature([Project, ProjectVersion])],
  controllers: [DownloadController],
  providers: [DownloadService],
})
export class DownloadModule {}
