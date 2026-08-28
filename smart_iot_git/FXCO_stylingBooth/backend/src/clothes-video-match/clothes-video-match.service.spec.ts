import { Test, TestingModule } from '@nestjs/testing';
import { ClothesVideoMatchService } from './clothes-video-match.service';

describe('ClothesVideoMatchService', () => {
  let service: ClothesVideoMatchService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [ClothesVideoMatchService],
    }).compile();

    service = module.get<ClothesVideoMatchService>(ClothesVideoMatchService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });
});
