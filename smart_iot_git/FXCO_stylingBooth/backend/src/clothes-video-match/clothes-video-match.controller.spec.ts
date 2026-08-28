import { Test, TestingModule } from '@nestjs/testing';
import { ClothesVideoMatchController } from './clothes-video-match.controller';

describe('ClothesVideoMatchController', () => {
  let controller: ClothesVideoMatchController;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [ClothesVideoMatchController],
    }).compile();

    controller = module.get<ClothesVideoMatchController>(ClothesVideoMatchController);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
  });
});
