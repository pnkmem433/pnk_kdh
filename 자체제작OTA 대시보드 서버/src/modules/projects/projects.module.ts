import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';



import { ProjectsService } from './projects.service';
import { ProjectsController } from './projects.controller';

import { JwtModule } from '../../jwt/jwt.module';
import { JwtStrategy } from '../../jwt/jwt.strategy';

import { Project } from '../../entites/project.entity';

@Module({
  imports: [
    JwtModule,
    TypeOrmModule.forFeature([Project]),
  ],
  providers: [ProjectsService, JwtStrategy],
  controllers: [ProjectsController],
  exports: [ProjectsService],
})
export class ProjectsModule { }
