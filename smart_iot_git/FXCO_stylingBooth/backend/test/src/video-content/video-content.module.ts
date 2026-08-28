import { Module } from '@nestjs/common';
import { VideoContentController } from './video-content.controller';
import { VideoContentService } from './video-content.service';
import { TypeOrmModule } from '@nestjs/typeorm';
import { VideoContent } from 'src/entities/videoContent.entity';

@Module({
  imports: [TypeOrmModule.forFeature([VideoContent])],
  controllers: [VideoContentController],
  providers: [VideoContentService]
})
export class VideoContentModule {}
