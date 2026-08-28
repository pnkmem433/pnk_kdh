import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { ClothesVideoMatch } from 'src/entities/clothesVideoMatch.entity';
import { ClothesTypes } from 'src/entities/clothesTypes.entity';

@Injectable()
export class ClothesVideoMatchService {
    constructor(
        @InjectRepository(ClothesVideoMatch)
        private readonly clothesVideoMatchRepository: Repository<ClothesVideoMatch>,
    ) { }

    async create(data: Partial<ClothesVideoMatch>): Promise<ClothesVideoMatch> {
        const entity = this.clothesVideoMatchRepository.create(data);
        return this.clothesVideoMatchRepository.save(entity);
    }

    async findAll(): Promise<ClothesVideoMatch[]> {
        return this.clothesVideoMatchRepository.find();
    }

    async findOne(seq: number): Promise<ClothesVideoMatch | null> {
        return this.clothesVideoMatchRepository.findOne({ where: { seq } });
    }

    async update(seq: number, data: Partial<ClothesVideoMatch>): Promise<ClothesVideoMatch | null> {
        await this.clothesVideoMatchRepository.update(seq, data);
        return this.findOne(seq);
    }

    async remove(seq: number): Promise<void> {
        await this.clothesVideoMatchRepository.delete(seq);
    }

  async findVideoByClothesSeq(clothesSeq: number): Promise<number | null> {
    const match = await this.clothesVideoMatchRepository.findOne({
      where: { clothes_seq: clothesSeq },
      relations: ['video']
    });

    return match?.video_seq || null;   
  }

async findVideoByClothesProductId(clothesProductId: string): Promise<ClothesVideoMatch | null> {
  const clothesType = await this.clothesVideoMatchRepository.manager.getRepository(ClothesTypes).findOne({
    where: { product_id: clothesProductId }
  });

  if (!clothesType) {
    return null;
  }

  const match = await this.clothesVideoMatchRepository.findOne({
    where: { clothes_seq: clothesType.seq },
  });

  return match || null;
}
}
