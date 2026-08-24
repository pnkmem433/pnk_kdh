import { Test, TestingModule } from '@nestjs/testing';
import { VideoContentController } from './video-content.controller';

describe('VideoContentController', () => {
  let controller: VideoContentController;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      controllers: [VideoContentController],
    }).compile();

    controller = module.get<VideoContentController>(VideoContentController);
  });

  it('should be defined', () => {
    expect(controller).toBeDefined();
  });
});
