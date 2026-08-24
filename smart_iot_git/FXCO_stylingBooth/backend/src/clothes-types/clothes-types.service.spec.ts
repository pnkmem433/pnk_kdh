import { Test, TestingModule } from '@nestjs/testing';
import { ClothesTypesService } from './clothes-types.service';

describe('ClothesTypesService', () => {
  let service: ClothesTypesService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [ClothesTypesService],
    }).compile();

    service = module.get<ClothesTypesService>(ClothesTypesService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });
});
