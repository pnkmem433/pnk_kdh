import { Test, TestingModule } from '@nestjs/testing';
import { SystemParameterService } from './system-parameter.service';

describe('SystemParameterService', () => {
  let service: SystemParameterService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [SystemParameterService],
    }).compile();

    service = module.get<SystemParameterService>(SystemParameterService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });
});
