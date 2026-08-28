import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { VideoRoomDoorSensor } from 'src/entities/videoRoomDoorSensor.entity';

@Injectable()
export class VideoRoomDoorSensorService {
  constructor(
    @InjectRepository(VideoRoomDoorSensor)
    private readonly videoRoomDoorSensorRepository: Repository<VideoRoomDoorSensor>,
  ) { }

  async create(data: Partial<VideoRoomDoorSensor>): Promise<VideoRoomDoorSensor> {
    const entity = this.videoRoomDoorSensorRepository.create(data);
    entity.video_start_time = new Date();
    entity.video_end_time = new Date();
    entity.event_time = new Date();
    return this.videoRoomDoorSensorRepository.save(entity);
  }

  async findAll(): Promise<VideoRoomDoorSensor[]> {
    return this.videoRoomDoorSensorRepository.find();
  }

  async findOne(seq: number): Promise<VideoRoomDoorSensor | null> {
    return this.videoRoomDoorSensorRepository.findOne({ where: { seq } });
  }

  async update(seq: number, data: Partial<VideoRoomDoorSensor>): Promise<VideoRoomDoorSensor | null> {
    await this.videoRoomDoorSensorRepository.update(seq, data);
    return this.findOne(seq);
  }

  async remove(seq: number): Promise<void> {
    await this.videoRoomDoorSensorRepository.delete(seq);
  }

  async setDoorOpen(isOpened: number, seq: number) {
    return this.videoRoomDoorSensorRepository.update({ seq: seq }, { is_opened: isOpened });
  }

  async getLastRecord(sessionSeq: number): Promise<VideoRoomDoorSensor | null> {
    return this.videoRoomDoorSensorRepository.findOne({ where: { session: { seq: sessionSeq } }, order: { seq: 'DESC' } });
  }
}
