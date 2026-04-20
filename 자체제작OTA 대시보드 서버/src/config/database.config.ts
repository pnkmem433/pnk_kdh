import { TypeOrmModuleOptions } from '@nestjs/typeorm';
import * as path from 'path';
import { User } from '../entites/user.entity';
import { Project } from '../entites/project.entity';
import { ProjectVersion } from '../entites/version.entity';

function parseBoolean(value: string | undefined, fallback: boolean): boolean {
  if (value === undefined) {
    return fallback;
  }

  const normalized = value.trim().toLowerCase();
  return normalized === '1' || normalized === 'true' || normalized === 'yes';
}

export function createDatabaseConfig(): TypeOrmModuleOptions {
  const dbType = (process.env.DB_TYPE ?? 'sqlite').trim().toLowerCase();
  const synchronize = parseBoolean(process.env.DB_SYNCHRONIZE, dbType === 'sqlite');
  const logging = parseBoolean(process.env.DB_LOGGING, false);

  if (dbType === 'sqlite') {
    const databaseFile = path.resolve(process.cwd(), process.env.DB_DATABASE ?? 'local-ota.sqlite');
    return {
      type: 'sqlite',
      database: databaseFile,
      entities: [User, Project, ProjectVersion],
      synchronize,
      logging,
    };
  }

  return {
    type: 'mysql',
    host: process.env.DB_HOST ?? '127.0.0.1',
    port: parseInt(process.env.DB_PORT ?? '3306', 10),
    username: process.env.DB_USERNAME ?? 'root',
    password: process.env.DB_PASSWORD ?? '',
    database: process.env.DB_DATABASE ?? 'arduino_ota',
    entities: [User, Project, ProjectVersion],
    synchronize,
    logging,
  };
}
