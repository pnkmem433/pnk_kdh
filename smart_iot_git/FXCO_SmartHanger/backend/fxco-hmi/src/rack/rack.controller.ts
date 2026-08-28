import { Controller, Get, Param, ParseIntPipe } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiParam } from '@nestjs/swagger';
import { RackService } from './rack.service';
import { Rack } from '../entities/rack.entity';

@ApiTags('rack')
@Controller('rack')
export class RackController {
  constructor(private readonly rackService: RackService) {}

  @Get()
  @ApiOperation({ 
    summary: '모든 붙을 행거랙 데이터 조회', 
    description: '데이터베이스에서 모든 붙을 행거랙 정보를 조회합니다.' 
  })
  @ApiResponse({ status: 200, description: '붙을 행거랙 목록 조회 성공', type: [Rack] })
  async getAllRacks(): Promise<Rack[]> {
    return await this.rackService.findAll();
  }

  @Get(':seq')
  @ApiOperation({ 
    summary: '특정 붙을 행거랙 데이터 조회', 
    description: '시퀀스 번호를 통해 특정 붙을 행거랙 정보를 조회합니다.' 
  })
  @ApiParam({ name: 'seq', description: '붙을 행거랙 시퀀스 번호', type: Number })
  @ApiResponse({ status: 200, description: '붙을 행거랙 조회 성공', type: Rack })
  @ApiResponse({ status: 404, description: '붙을 행거랙을 찾을 수 없음' })
  async getRackBySeq(@Param('seq', ParseIntPipe) seq: number): Promise<Rack | null> {
    return await this.rackService.findOne(seq);
  }
}
