import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { VideoContent } from 'src/entities/videoContent.entity';

@Injectable()
export class VideoContentService {
  constructor(
    @InjectRepository(VideoContent)
    private readonly videoContentRepository: Repository<VideoContent>,
  ) {}

  async create(data: Partial<VideoContent>): Promise<VideoContent> {
    const entity = this.videoContentRepository.create(data);
    return this.videoContentRepository.save(entity);
  }

  async findAll(): Promise<VideoContent[]> {
    return this.videoContentRepository.find();
  }

  async findOne(seq: number): Promise<VideoContent | null> {
    return this.videoContentRepository.findOne({ where: { seq } });
  }

  async update(seq: number, data: Partial<VideoContent>): Promise<VideoContent | null> {
    await this.videoContentRepository.update(seq, data);
    return this.findOne(seq);
  }

  async remove(seq: number): Promise<void> {
    await this.videoContentRepository.delete(seq);
  }
}
