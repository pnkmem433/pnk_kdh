import { Test, TestingModule } from '@nestjs/testing';
import { AppController } from './app.controller';

describe('AppController', () => {
  let appController: AppController;

  beforeEach(async () => {
    const app: TestingModule = await Test.createTestingModule({
      controllers: [AppController],
    }).compile();

    appController = app.get<AppController>(AppController);
  });

  describe('root', () => {
    it('should return "FXCO HMI 서버가 정상적으로 실행 중입니다."', () => {
      expect(appController.getHello()).toBe('FXCO HMI 서버가 정상적으로 실행 중입니다.');
    });
  });
});




