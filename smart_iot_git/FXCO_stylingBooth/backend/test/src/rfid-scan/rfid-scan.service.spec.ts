import { Test, TestingModule } from '@nestjs/testing';
import { RfidScanService } from './rfid-scan.service';

describe('RfidScanService', () => {
  let service: RfidScanService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [RfidScanService],
    }).compile();

    service = module.get<RfidScanService>(RfidScanService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });
});
