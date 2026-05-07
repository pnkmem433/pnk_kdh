import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ConfigModule } from '@nestjs/config';

import { UsersModule } from './modules/users/users.module';
import { AuthModule } from './modules/auth/auth.module';
import { ProjectsModule } from './modules/projects/projects.module';
import { ProjectVersionModule } from './modules/versions/versions.module';
import { DownloadModule } from './modules/firmwareDownload/download.module';
import { createDatabaseConfig } from './config/database.config';

@Module({
  imports: [
    ConfigModule.forRoot({ isGlobal: true }),
    TypeOrmModule.forRoot(createDatabaseConfig()),
    UsersModule,
    AuthModule,
    ProjectsModule,
    ProjectVersionModule,
    DownloadModule,
  ],
})
export class AppModule {}
