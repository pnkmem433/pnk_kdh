import { Test, TestingModule } from '@nestjs/testing';
import { SystemParameterController } from './system-parameter.controller';

describe('SystemParameterController', () => {
  let controller: SystemParameterController;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [SystemParameterController],
    }).compile();

    controller = module.get<SystemParameterController>(SystemParameterController);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
  });
});
