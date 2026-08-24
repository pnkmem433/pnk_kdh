import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { FittingRoomDoorSensor } from 'src/entities/fittingRoomDoorSensor.entity';

@Injectable()
export class FittingRoomDoorSensorService {
  constructor(
    @InjectRepository(FittingRoomDoorSensor)
    private readonly fittingRoomDoorSensorRepository: Repository<FittingRoomDoorSensor>,
  ) {}

  async create(data: Partial<FittingRoomDoorSensor>): Promise<FittingRoomDoorSensor> {
    data.event_time = new Date();
    const entity = this.fittingRoomDoorSensorRepository.create(data);
    return this.fittingRoomDoorSensorRepository.save(entity);
  }

  async findAll(): Promise<FittingRoomDoorSensor[]> {
    return this.fittingRoomDoorSensorRepository.find();
  }

  async findOne(seq: number): Promise<FittingRoomDoorSensor | null> {
    return this.fittingRoomDoorSensorRepository.findOne({ where: { seq } });
  }

  async update(seq: number, data: Partial<FittingRoomDoorSensor>): Promise<FittingRoomDoorSensor | null> {
    await this.fittingRoomDoorSensorRepository.update(seq, data);
    return this.findOne(seq);
  }

  async remove(seq: number): Promise<void> {
    await this.fittingRoomDoorSensorRepository.delete(seq);
  }

  async setDoorOpen(isOpened: number, seq: number) {
    return this.fittingRoomDoorSensorRepository.update({ seq: seq }, { is_opened: isOpened });
  }

  async getLastRecord(sessionSeq: number): Promise<FittingRoomDoorSensor | null> {
    return this.fittingRoomDoorSensorRepository.findOne({ where: { session: { seq: sessionSeq } }, order: { seq: 'DESC' } });
  }
}
