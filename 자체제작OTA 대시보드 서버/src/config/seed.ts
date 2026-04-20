import { DataSource } from 'typeorm';
import * as bcrypt from 'bcryptjs';
import { User } from '../entites/user.entity';
import { Project } from '../entites/project.entity';
import { ProjectVersion } from '../entites/version.entity';

function isEnabled(value: string | undefined, fallback: boolean): boolean {
  if (value === undefined) {
    return fallback;
  }

  const normalized = value.trim().toLowerCase();
  return normalized === '1' || normalized === 'true' || normalized === 'yes';
}

export async function seedLocalDataSource(dataSource: DataSource) {
  const userRepo = dataSource.getRepository(User);
  const projectRepo = dataSource.getRepository(Project);
  const projectVersionRepo = dataSource.getRepository(ProjectVersion);

  const defaultUserId = process.env.SEED_USER_ID ?? 'admin';
  const defaultUserName = process.env.SEED_USER_NAME ?? 'admin';
  const defaultUserPassword = process.env.SEED_USER_PASSWORD ?? 'admin1234';
  const defaultProjectName = process.env.SEED_PROJECT_NAME ?? 'smartplug';
  const defaultProjectId = parseInt(process.env.SEED_PROJECT_ID ?? '10', 10);
  const defaultProjectUuid =
    process.env.SEED_PROJECT_UUID ?? '41742426-d035-4756-ba17-f542ea75ab02';

  const resetForLocal = isEnabled(process.env.SEED_RESET_LOCAL_DATA, true);
  const dbType = (process.env.DB_TYPE ?? 'sqlite').trim().toLowerCase();

  if (dbType === 'sqlite' && resetForLocal) {
    await projectVersionRepo.clear();
    await projectRepo.clear();
    await userRepo.clear();
    console.log('[seed] local sqlite data cleared');
  }

  const user = userRepo.create({
    id: defaultUserId,
    name: defaultUserName,
    password: await bcrypt.hash(defaultUserPassword, 10),
  });
  await userRepo.save(user);
  console.log(`[seed] created user: ${defaultUserId}`);

  const project = projectRepo.create({
    id: defaultProjectId,
    uuid: defaultProjectUuid,
    name: defaultProjectName,
    userSeq: user,
  });
  await projectRepo.save(project);
  console.log(`[seed] created project: ${defaultProjectName}`);
}
