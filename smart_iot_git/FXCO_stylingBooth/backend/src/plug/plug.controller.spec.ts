import { Test, TestingModule } from '@nestjs/testing';
import { PlugController } from './plug.controller';

describe('PlugController', () => {
  let controller: PlugController;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [PlugController],
    }).compile();

    controller = module.get<PlugController>(PlugController);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
  });
});
