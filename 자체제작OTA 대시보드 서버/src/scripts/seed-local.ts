import * as dotenv from 'dotenv';
dotenv.config();

import dataSource from '../../data-source';
import { seedLocalDataSource } from '../config/seed';

async function run() {
  await dataSource.initialize();
  await seedLocalDataSource(dataSource);
  await dataSource.destroy();
  console.log('[seed] local seed completed');
}

run().catch((error) => {
  console.error('[seed] local seed failed');
  console.error(error);
  process.exit(1);
});
