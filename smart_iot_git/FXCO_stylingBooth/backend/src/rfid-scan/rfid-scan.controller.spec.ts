import { Test, TestingModule } from '@nestjs/testing';
import { RfidScanController } from './rfid-scan.controller';

describe('RfidScanController', () => {
  let controller: RfidScanController;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [RfidScanController],
    }).compile();

    controller = module.get<RfidScanController>(RfidScanController);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
  });
});
