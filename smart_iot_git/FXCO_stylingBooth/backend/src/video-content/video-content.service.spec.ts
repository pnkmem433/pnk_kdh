import { Test, TestingModule } from '@nestjs/testing';
import { VideoContentService } from './video-content.service';

describe('VideoContentService', () => {
  let service: VideoContentService;

  beforeEach(async () => {
    const module: TestingModule = await Test.createTestingModule({
      providers: [VideoContentService],
    }).compile();

    service = module.get<VideoContentService>(VideoContentService);
  });

  it('should be defined', () => {
    expect(service).toBeDefined();
  });
});
