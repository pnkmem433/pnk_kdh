import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Clothes } from '../entities/clothes.entity';

@Injectable()
export class ClothesService {
  constructor(
    @InjectRepository(Clothes)
    private clothesRepository: Repository<Clothes>,
  ) {}

  async findAll(): Promise<Clothes[]> {
    return await this.clothesRepository.find();
  }

  async findOne(seq: number): Promise<Clothes | null> {
    return await this.clothesRepository.findOne({ where: { seq } });
  }
}




