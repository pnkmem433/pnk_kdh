import { Test, TestingModule } from '@nestjs/testing';
import { VideoRoomDoorSensorService } from './video-room-door-sensor.service';

describe('VideoRoomDoorSensorService', () => {
  let service: VideoRoomDoorSensorService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [VideoRoomDoorSensorService],
    }).compile();

    service = module.get<VideoRoomDoorSensorService>(VideoRoomDoorSensorService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });
});
