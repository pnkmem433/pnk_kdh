import { Controller, Get, Param, ParseIntPipe } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiParam } from '@nestjs/swagger';
import { ClothesService } from './clothes.service';
import { Clothes } from '../entities/clothes.entity';

@ApiTags('clothes')
@Controller('clothes')
export class ClothesController {
  constructor(private readonly clothesService: ClothesService) {}

  @Get()
  @ApiOperation({ summary: '모든 의류 데이터 조회', description: '데이터베이스에서 모든 의류 정보를 조회합니다.' })
  @ApiResponse({ status: 200, description: '의류 목록 조회 성공', type: [Clothes] })
  async getAllClothes(): Promise<Clothes[]> {
    return await this.clothesService.findAll();
  }

  @Get(':seq')
  @ApiOperation({ summary: '특정 의류 데이터 조회', description: '시퀀스 번호를 통해 특정 의류 정보를 조회합니다.' })
  @ApiParam({ name: 'seq', description: '의류 시퀀스 번호', type: Number })
  @ApiResponse({ status: 200, description: '의류 조회 성공', type: Clothes })
  @ApiResponse({ status: 404, description: '의류를 찾을 수 없음' })
  async getClothesBySeq(@Param('seq', ParseIntPipe) seq: number): Promise<Clothes | null> {
    return await this.clothesService.findOne(seq);
  }
}




