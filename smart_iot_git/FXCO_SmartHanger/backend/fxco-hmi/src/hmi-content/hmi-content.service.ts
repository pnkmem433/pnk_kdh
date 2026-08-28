import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { HmiContent } from '../entities/hmi-content.entity';

@Injectable()
export class HmiContentService {
  constructor(
    @InjectRepository(HmiContent)
    private hmiContentRepository: Repository<HmiContent>,
  ) {}

  async findAll(): Promise<HmiContent[]> {
    return await this.hmiContentRepository.find();
  }

  async findOne(seq: number): Promise<HmiContent | null> {
    return await this.hmiContentRepository.findOne({ where: { seq } });
  }
}


