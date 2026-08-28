import { Test, TestingModule } from '@nestjs/testing';
import { PlugService } from './plug.service';

describe('PlugService', () => {
  let service: PlugService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [PlugService],
    }).compile();

    service = module.get<PlugService>(PlugService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });
});
