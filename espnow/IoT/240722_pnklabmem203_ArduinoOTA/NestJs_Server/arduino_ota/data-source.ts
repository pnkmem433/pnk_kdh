import { DataSource } from 'typeorm';
import { User } from './src/entites/user.entity';
import { Project } from './src/entites/project.entity';
import { ProjectVersion } from './src/entites/version.entity';

const dataSource = new DataSource({
  type: 'mysql',
  host: process.env.DB_HOST,
  port: parseInt(process.env.DB_PORT, 10),
  username: process.env.DB_USERNAME,
  password: process.env.DB_PASSWORD,
  database: process.env.DB_DATABASE,
  entities: [
    User,
    Project,
    ProjectVersion
  ],
  synchronize: false,
  logging: true,
});

export default dataSource;