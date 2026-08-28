import { Test, TestingModule } from '@nestjs/testing';
import { FittingRoomDoorSensorService } from './fitting-room-door-sensor.service';

describe('FittingRoomDoorSensorService', () => {
  let service: FittingRoomDoorSensorService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [FittingRoomDoorSensorService],
    }).compile();

    service = module.get<FittingRoomDoorSensorService>(FittingRoomDoorSensorService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });
});
