import { Controller, Get, Param, ParseIntPipe } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiParam } from '@nestjs/swagger';
import { HmiContentService } from './hmi-content.service';
import { HmiContent } from '../entities/hmi-content.entity';

@ApiTags('hmi-content')
@Controller('hmi-content')
export class HmiContentController {
  constructor(private readonly hmiContentService: HmiContentService) {}

  @Get()
  @ApiOperation({ summary: '모든 HMI 콘텐츠 데이터 조회', description: '데이터베이스에서 모든 HMI 콘텐츠 정보를 조회합니다.' })
  @ApiResponse({ status: 200, description: 'HMI 콘텐츠 목록 조회 성공', type: [HmiContent] })
  async getAllHmiContents(): Promise<HmiContent[]> {
    return await this.hmiContentService.findAll();
  }

  @Get(':seq')
  @ApiOperation({ summary: '특정 HMI 콘텐츠 데이터 조회', description: '시퀀스 번호를 통해 특정 HMI 콘텐츠 정보를 조회합니다.' })
  @ApiParam({ name: 'seq', description: 'HMI 콘텐츠 시퀀스 번호', type: Number })
  @ApiResponse({ status: 200, description: 'HMI 콘텐츠 조회 성공', type: HmiContent })
  @ApiResponse({ status: 404, description: 'HMI 콘텐츠를 찾을 수 없음' })
  async getHmiContentBySeq(@Param('seq', ParseIntPipe) seq: number): Promise<HmiContent | null> {
    return await this.hmiContentService.findOne(seq);
  }
}


