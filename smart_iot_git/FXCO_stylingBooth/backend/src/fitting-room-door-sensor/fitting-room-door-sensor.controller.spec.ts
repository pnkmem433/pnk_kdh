import { Test, TestingModule } from '@nestjs/testing';
import { FittingRoomDoorSensorController } from './fitting-room-door-sensor.controller';

describe('FittingRoomDoorSensorController', () => {
  let controller: FittingRoomDoorSensorController;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [FittingRoomDoorSensorController],
    }).compile();

    controller = module.get<FittingRoomDoorSensorController>(FittingRoomDoorSensorController);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
  });
});
