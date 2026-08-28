import { Controller, Get, Param, ParseIntPipe, Patch, Body } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiParam } from '@nestjs/swagger';
import { HangerlegService } from './hangerleg.service';
import { Hangerleg } from '../entities/hangerleg.entity';
import { ReplaceHangerlegDto } from './dto/replace-hangerleg.dto';

@ApiTags('hangerleg')
@Controller('hangerleg')
export class HangerlegController {
  constructor(private readonly hangerlegService: HangerlegService) {}

  @Get()
  @ApiOperation({ summary: '모든 행거랙 데이터 조회', description: '데이터베이스에서 모든 행거랙 정보를 조회합니다.' })
  @ApiResponse({ status: 200, description: '행거랙 목록 조회 성공', type: [Hangerleg] })
  async getAllHangerlegs(): Promise<Hangerleg[]> {
    return await this.hangerlegService.findAll();
  }

  @Get(':seq')
  @ApiOperation({ summary: '특정 행거랙 데이터 조회', description: '시퀀스 번호를 통해 특정 행거랙 정보를 조회합니다.' })
  @ApiParam({ name: 'seq', description: '행거랙 시퀀스 번호', type: Number })
  @ApiResponse({ status: 200, description: '행거랙 조회 성공', type: Hangerleg })
  @ApiResponse({ status: 404, description: '행거랙을 찾을 수 없음' })
  async getHangerlegBySeq(@Param('seq', ParseIntPipe) seq: number): Promise<Hangerleg | null> {
    return await this.hangerlegService.findOne(seq);
  }

  @Patch('replace')
  @ApiOperation({ 
    summary: '스마트 헹거랙 교체', 
    description: '특정 위치(rack_seq, position)에 있는 기존 헹거랙을 해제하고, 새로운 헹거랙을 해당 위치에 배치합니다. 기존 헹거랙의 rack_seq와 position은 null로 설정됩니다.' 
  })
  @ApiResponse({ status: 200, description: '헹거랙 교체 성공', type: Hangerleg })
  @ApiResponse({ status: 404, description: '헹거랙 또는 붙을 행거랙을 찾을 수 없음' })
  async replaceHangerleg(@Body() dto: ReplaceHangerlegDto): Promise<Hangerleg> {
    return await this.hangerlegService.replaceHangerleg(dto);
  }
}


