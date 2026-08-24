import { Test, TestingModule } from '@nestjs/testing';
import { ClothesTypesController } from './clothes-types.controller';

describe('ClothesTypesController', () => {
  let controller: ClothesTypesController;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [ClothesTypesController],
    }).compile();

    controller = module.get<ClothesTypesController>(ClothesTypesController);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
  });
});
