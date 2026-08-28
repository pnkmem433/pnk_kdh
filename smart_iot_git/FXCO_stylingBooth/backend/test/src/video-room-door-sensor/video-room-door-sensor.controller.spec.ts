import { Test, TestingModule } from '@nestjs/testing';
import { VideoRoomDoorSensorController } from './video-room-door-sensor.controller';

describe('VideoRoomDoorSensorController', () => {
  let controller: VideoRoomDoorSensorController;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [VideoRoomDoorSensorController],
    }).compile();

    controller = module.get<VideoRoomDoorSensorController>(VideoRoomDoorSensorController);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
  });
});
