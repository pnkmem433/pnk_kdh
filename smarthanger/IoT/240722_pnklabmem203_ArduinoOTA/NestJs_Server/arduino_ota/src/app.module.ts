import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ConfigModule } from '@nestjs/config';

import { User } from './entites/user.entity';
import { Project } from './entites/project.entity';
import { ProjectVersion } from './entites/version.entity';

import { UsersModule } from './modules/users/users.module';
import { AuthModule } from './modules/auth/auth.module';
import { ProjectsModule } from './modules/projects/projects.module';
import { ProjectVersionModule } from './modules/versions/versions.module';
import { DownloadModule } from './modules/firmwareDownload/download.module';

@Module({
  imports: [
    ConfigModule.forRoot({ isGlobal: true }),
    TypeOrmModule.forRoot({
      type: 'mysql',
      host: process.env.DB_HOST,
      port: parseInt(process.env.DB_PORT, 10),
      username: process.env.DB_USERNAME,
      password: process.env.DB_PASSWORD,
      database: process.env.DB_DATABASE,
      entities: [User, Project, ProjectVersion],
      synchronize: false,
      logging: ['query', 'error', 'schema', 'warn', 'info'], // 로그 레벨 추가
    }),
    UsersModule,
    AuthModule,
    ProjectsModule,
    ProjectVersionModule,
    DownloadModule,
  ],
})
export class AppModule { }
