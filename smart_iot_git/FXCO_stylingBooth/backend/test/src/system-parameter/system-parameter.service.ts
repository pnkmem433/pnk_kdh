import { Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { SystemParameter } from 'src/entities/systemParameter.entity';
import { Repository } from 'typeorm';

@Injectable()
export class SystemParameterService {
  constructor(
    @InjectRepository(SystemParameter)
    private readonly systemParameterRepository: Repository<SystemParameter>,
  ) {}


  async findAll(): Promise<SystemParameter[]> {
    return this.systemParameterRepository.find();
  }

  async findOneBySeq(seq: number): Promise<SystemParameter | null> {
    return this.systemParameterRepository.findOne({ where: { seq } });
  }

  async paramUpdate(seq: number, parameter: number): Promise<SystemParameter> {
    const systemParameter = await this.systemParameterRepository.findOne({ where: { seq } });
    if (!systemParameter) {
      throw new NotFoundException('System parameter not found');
    }
    systemParameter.parameter = parameter;
    return this.systemParameterRepository.save(systemParameter);
  }
}
