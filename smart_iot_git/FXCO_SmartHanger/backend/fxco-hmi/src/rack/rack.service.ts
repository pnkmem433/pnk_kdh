import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Rack } from '../entities/rack.entity';

@Injectable()
export class RackService {
  constructor(
    @InjectRepository(Rack)
    private rackRepository: Repository<Rack>,
  ) {}

  async findAll(): Promise<Rack[]> {
    return await this.rackRepository.find();
  }

  async findOne(seq: number): Promise<Rack | null> {
    return await this.rackRepository.findOne({ where: { seq } });
  }

  async findByRackNumber(rackNumber: string): Promise<Rack | null> {
    return await this.rackRepository.findOne({ where: { rackNumber } });
  }
}
