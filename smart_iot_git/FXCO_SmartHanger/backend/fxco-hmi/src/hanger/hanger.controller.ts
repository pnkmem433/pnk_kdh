import { Controller, Get, Param, ParseIntPipe, Patch, Body } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiParam } from '@nestjs/swagger';
import { HangerService } from './hanger.service';
import { HangerResponseDto } from './dto/hanger-response.dto';
import { ReplaceHangerDto } from './dto/replace-hanger.dto';
import { MatchClothesDto } from './dto/match-clothes.dto';
import { Hanger } from '../entities/hanger.entity';

@ApiTags('hanger')
@Controller('hanger')
export class HangerController {
  constructor(private readonly hangerService: HangerService) {}

  @Get()
  @ApiOperation({ 
    summary: '모든 행거 데이터 조회', 
    description: '데이터베이스에서 모든 행거 정보를 조회합니다. hanger_log와 연동하여 현재 위치(hangerlegSeqCurrent)도 함께 반환합니다.' 
  })
  @ApiResponse({ status: 200, description: '행거 목록 조회 성공', type: [HangerResponseDto] })
  async getAllHangers(): Promise<HangerResponseDto[]> {
    return await this.hangerService.findAll();
  }

  @Get(':seq')
  @ApiOperation({ 
    summary: '특정 행거 데이터 조회', 
    description: '시퀀스 번호를 통해 특정 행거 정보를 조회합니다. hanger_log와 연동하여 현재 위치(hangerlegSeqCurrent)도 함께 반환합니다.' 
  })
  @ApiParam({ name: 'seq', description: '행거 시퀀스 번호', type: Number })
  @ApiResponse({ status: 200, description: '행거 조회 성공', type: HangerResponseDto })
  @ApiResponse({ status: 404, description: '행거를 찾을 수 없음' })
  async getHangerBySeq(@Param('seq', ParseIntPipe) seq: number): Promise<HangerResponseDto | null> {
    return await this.hangerService.findOne(seq);
  }

  @Patch('replace')
  @ApiOperation({ 
    summary: '스마트 헹거 교체', 
    description: '스마트 헹거의 위치(헹거랙)를 변경합니다. 헹거 UUID와 헹거랙 UUID를 입력받아 헹거의 hangerleg_seq를 업데이트합니다.' 
  })
  @ApiResponse({ status: 200, description: '헹거 교체 성공', type: Hanger })
  @ApiResponse({ status: 404, description: '헹거 또는 헹거랙을 찾을 수 없음' })
  async replaceHanger(@Body() dto: ReplaceHangerDto): Promise<Hanger> {
    return await this.hangerService.replaceHanger(dto);
  }

  @Patch('match-clothes')
  @ApiOperation({ 
    summary: '헹거와 옷 매칭', 
    description: '스마트 헹거에 의류를 연결합니다. 헹거 UUID와 의류 시퀀스 번호를 입력받아 헹거의 clothes_seq를 업데이트합니다.' 
  })
  @ApiResponse({ status: 200, description: '옷 매칭 성공', type: Hanger })
  @ApiResponse({ status: 404, description: '헹거 또는 의류를 찾을 수 없음' })
  async matchClothes(@Body() dto: MatchClothesDto): Promise<Hanger> {
    return await this.hangerService.matchClothes(dto);
  }
}

