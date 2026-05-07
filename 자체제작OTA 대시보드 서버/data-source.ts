import { DataSource } from 'typeorm';
import { createDatabaseConfig } from './src/config/database.config';

const dataSource = new DataSource(createDatabaseConfig() as any);

export default dataSource;
