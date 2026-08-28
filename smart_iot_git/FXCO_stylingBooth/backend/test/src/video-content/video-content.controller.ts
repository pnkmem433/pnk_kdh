import { Controller, Get, Post, Body, Param, Patch, Delete } from '@nestjs/common';
import { VideoContentService } from './video-content.service';
import { VideoContent } from 'src/entities/videoContent.entity';
import { ApiOperation, ApiTags, ApiParam, ApiBody } from '@nestjs/swagger';

@Controller('video-content')
@ApiTags('비디오 컨텐츠 API')
export class VideoContentController {
  constructor(private readonly videoContentService: VideoContentService) {}

  @ApiOperation({ summary: '비디오 컨텐츠 주소 목록 생성' })
  @Post()
  @ApiBody({
    description: '비디오 컨텐츠 생성',
    examples: {
      기본예시: {
        value: {
          video_url: 'https://example.com/video.mp4'
        }
      }
    }
  })
  async create(@Body() data: Partial<VideoContent>) {
    return this.videoContentService.create(data);
  }

  @ApiOperation({ summary: '비디오 컨텐츠 목록 조회' })
  @Get()
  async findAll() {
    return this.videoContentService.findAll();
  }

  @ApiOperation({ summary: '비디오 컨텐츠 주소 상세 조회' })
  @Get(':seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '비디오 컨텐츠 고유번호' })
  async findOne(@Param('seq') seq: number) {
    return this.videoContentService.findOne(Number(seq));
  }

  @ApiOperation({ summary: '비디오 컨텐츠 주소 수정' })
  @Patch(':seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '비디오 컨텐츠 고유번호' })
  async update(@Param('seq') seq: number, @Body() data: Partial<VideoContent>) {
    return this.videoContentService.update(Number(seq), data);
  }

  @ApiOperation({ summary: '비디오 컨텐츠 주소 삭제' })
  @Delete(':seq')
  @ApiParam({ name: 'seq', type: Number, example: 1, description: '비디오 컨텐츠 고유번호' })
  async remove(@Param('seq') seq: number) {
    await this.videoContentService.remove(Number(seq));
    return { deleted: true };
  }

  
}
