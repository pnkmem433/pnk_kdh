import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Hanger } from '../entities/hanger.entity';
import { Clothes } from '../entities/clothes.entity';

@Injectable()
export class ClothesService {
  constructor(
    @InjectRepository(Hanger)
    private readonly hangerRepo: Repository<Hanger>,
    @InjectRepository(Clothes)
    private readonly clothesRepo: Repository<Clothes>,
  ) {}

async attachClothes(hangerUuid: string, clothesTag: string) {
  const hanger = await this.hangerRepo.findOne({
    where: { uuid: hangerUuid },
    relations: ['clothes'],
  });
  if (!hanger) return { ignored: true, reason: 'Hanger not found' };

  const clothes = await this.clothesRepo.findOneBy({ tag: clothesTag });
  if (!clothes) return { ignored: true, reason: 'Clothes not found' };

  // 이미 같은 옷이면 무시
  if (hanger.clothes?.seq === clothes.seq)
    return { ignored: true, reason: 'This clothes is already on the hanger' };

  // 기존 옷 자동 해제 후 새 옷 연결
  hanger.clothes = clothes;
  await this.hangerRepo.save(hanger);

  return { success: true };
}

async detachClothes(hangerUuid: string) {
  const hanger = await this.hangerRepo.findOne({
    where: { uuid: hangerUuid },
    relations: ['clothes'],
  });
  if (!hanger) return { ignored: true, reason: 'Hanger not found' };

  if (!hanger.clothes) return { ignored: true, reason: 'No clothes attached to the hanger' };

  hanger.clothes = null;
  await this.hangerRepo.save(hanger);

  return { success: true };
}

}
