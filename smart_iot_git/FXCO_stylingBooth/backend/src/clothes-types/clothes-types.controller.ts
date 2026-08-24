import { Controller, Get, Post, Body, Param, Patch, Delete } from '@nestjs/common';
import { ClothesTypesService } from './clothes-types.service';
import { ClothesTypes } from 'src/entities/clothesTypes.entity';
import { ApiOperation, ApiParam, ApiTags } from '@nestjs/swagger';

@Controller('clothes-types')
@ApiTags('옷 타입 API')
export class ClothesTypesController {
  constructor(private readonly clothesTypesService: ClothesTypesService) {}

  @ApiOperation({ summary: '옷 타입 생성' })
  @ApiParam({ name: 'clothes_type', description: '옷 타입' })
  @Post()   
  async create(@Body() data: Partial<ClothesTypes>) {
    return this.clothesTypesService.create(data);
  }

  @ApiOperation({ summary: '옷 타입 조회' })
  @Get()
  async findAll() {
    return this.clothesTypesService.findAll();
  }

  @ApiOperation({ summary: '옷 타입 상세 조회' })
  @Get(':seq')
  async findOne(@Param('seq') seq: number) {
    return this.clothesTypesService.findOne(Number(seq));
  }

  @ApiOperation({ summary: '옷 타입 수정' })
  @Patch(':seq')
  async update(@Param('seq') seq: number, @Body() data: Partial<ClothesTypes>) {
    return this.clothesTypesService.update(Number(seq), data);
  }

  @ApiOperation({ summary: '옷 타입 삭제' })
  @Delete(':seq')
  async remove(@Param('seq') seq: number) {
    await this.clothesTypesService.remove(Number(seq));
    return { deleted: true };
  }
}
